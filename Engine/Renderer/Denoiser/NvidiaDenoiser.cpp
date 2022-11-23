#include <EnginePCH.h>
#include <NvidiaDenoiser.h>
#include <Application.h>
#include <CommandQueue.h>

#include <NRI.h>
#include <NRIDescs.hpp>
#include <Extensions/NRIWrapperD3D12.h>
#include <Extensions/NRIHelper.h>

#include <NRD.h>
#include <NRDIntegration.h>
#include <NRDIntegration.hpp>
#include <NRDSettings.h>

NrdIntegration NRD = NrdIntegration(3);

struct NriInterface
	: public nri::CoreInterface
	, public nri::HelperInterface
	, public nri::WrapperD3D12Interface
{};

nri::Device* m_NRIDevice;

NriInterface NRI;

nrd::CommonSettings commonSettings = {};

nri::CommandBuffer* nriCommandBuffer = nullptr;

std::string errorResult[] =
{
	"Success", "FAILURE", "INVALID ARGUMENT",
	"OUT_OF_MEMORY",
	"UNSUPPORTED",
	"DEVICE_LOST",
	"SWAPCHAIN_RESIZE",
	"MAX_NUM"
};

void NRDERRORCHECK(nri::Result result)
{
	if (nri::Result::SUCCESS == result) return;

	throw std::exception(errorResult[(int)result].c_str());
}

// Better use "true" if resources are not changing between frames (i.e. are not suballocated from a heap)
bool enableDescriptorCaching = true;

struct NRITextures
{
	nri::TextureTransitionBarrierDesc entryDescs = {};
	nri::Format entryFormat = {};
}nriTextures[nriTypes::size];


NvidiaDenoiser::NvidiaDenoiser()
{
	auto device = Application::Get().GetDevice();
	auto cq = Application::Get().GetCommandQueue();

	m_Adapter = Application::Get().GetAdapter(false).Get();

	nri::DeviceCreationD3D12Desc deviceDesc = {};
	deviceDesc.d3d12Device = device.Get();

	deviceDesc.d3d12PhysicalAdapter = m_Adapter.Get();
	deviceDesc.d3d12GraphicsQueue = cq->GetD3D12CommandQueue().Get();
	deviceDesc.enableNRIValidation = true;
	
	m_NRIDevice = nullptr;
	nri::Result nriResult = nri::CreateDeviceFromD3D12Device(deviceDesc, m_NRIDevice);

	// Get core functionality
	nriResult = nri::GetInterface(*m_NRIDevice,
		NRI_INTERFACE(nri::CoreInterface), (nri::CoreInterface*)&NRI);

	nriResult = nri::GetInterface(*m_NRIDevice,
		NRI_INTERFACE(nri::HelperInterface), (nri::HelperInterface*)&NRI);

	// Get appropriate "wrapper" extension (XXX - can be D3D11, D3D12 or VULKAN)
	nriResult = nri::GetInterface(*m_NRIDevice,
		NRI_INTERFACE(nri::WrapperD3D12Interface), (nri::WrapperD3D12Interface*)&NRI);

	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

}

NvidiaDenoiser::~NvidiaDenoiser()
{

	// Also NRD needs to be recreated on "resize"
	//NRD.Destroy();
}

void NvidiaDenoiser::Init(int width, int height, LightPass& lightPass)
{
	this->width = width;
	this->height = height;

	auto cq = Application::Get().GetCommandQueue();
	m_CommandList = cq->GetCommandList();

	const nrd::MethodDesc methodDescs[] =
	{
		// Put neeeded methods here, like:
		{ nrd::Method::REBLUR_DIFFUSE_SPECULAR, width, height },
	};

	nrd::DenoiserCreationDesc denoiserCreationDesc = {};
	denoiserCreationDesc.requestedMethods = methodDescs;
	denoiserCreationDesc.requestedMethodNum = 1;

	bool result = NRD.Initialize(denoiserCreationDesc, *m_NRIDevice, NRI, NRI);

	if (!result)
	{
		throw std::exception("Failed to create NRI interface");
	}	

	nri::CommandBufferD3D12Desc commandBufferDesc = {};
	commandBufferDesc.d3d12CommandList = (ID3D12GraphicsCommandList*)m_CommandList->GetGraphicsCommandList().Get();
	commandBufferDesc.d3d12CommandAllocator = m_commandAllocator.Get();

	NRI.CreateCommandBufferD3D12(*m_NRIDevice, commandBufferDesc, nriCommandBuffer);

	{//Normalrough
		nri::TextureD3D12Desc normalRoughDesc = {};

		normalRoughDesc.d3d12Resource = lightPass.GetTexture(nriTypes::inNormalRoughness).GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::inNormalRoughness].entryDescs;

		nriTextures[nriTypes::inNormalRoughness].entryFormat = 
			nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R10G10B10A2_UNORM);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, normalRoughDesc, (nri::Texture*&)entryDesc.texture));

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};
	{//Motion Vector
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = lightPass.GetTexture(nriTypes::inMV).GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::inMV].entryDescs;

		nriTextures[nriTypes::inMV].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, (nri::Texture*&)entryDesc.texture));

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};
	{//viewZ
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = lightPass.GetTexture(nriTypes::inViewZ).GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::inViewZ].entryDescs;

		nriTextures[nriTypes::inViewZ].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R32_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, (nri::Texture*&)entryDesc.texture));

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};
	{//Indirect Diffuse
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = lightPass.GetTexture(nriTypes::inIndirectDiffuse).GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::inIndirectDiffuse].entryDescs;

		nriTextures[nriTypes::inIndirectDiffuse].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, (nri::Texture*&)entryDesc.texture));

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};
	{//Indirect Specular
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = lightPass.GetTexture(nriTypes::inIndirectSpecular).GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::inIndirectSpecular].entryDescs;

		nriTextures[nriTypes::inIndirectSpecular].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, (nri::Texture*&)entryDesc.texture));

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};
	{//OUT Indirect Diffuse
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = lightPass.GetTexture(nriTypes::outIndirectDiffuse).GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::outIndirectDiffuse].entryDescs;

		nriTextures[nriTypes::outIndirectDiffuse].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, (nri::Texture*&)entryDesc.texture));

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE_STORAGE;
		entryDesc.nextLayout = nri::TextureLayout::GENERAL;
	};
	{//OUT Indirect Specular
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = lightPass.GetTexture(nriTypes::outIndirectSpecular).GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::outIndirectSpecular].entryDescs;

		nriTextures[nriTypes::outIndirectSpecular].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, (nri::Texture*&)entryDesc.texture));

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE_STORAGE;
		entryDesc.nextLayout = nri::TextureLayout::GENERAL;
	};
}


void NvidiaDenoiser::RenderFrame(const CameraCB &cam, LightPass& lightPass, int currentBackbufferIndex)
{
	commonSettings = {};

	/*
	memcpy(commonSettings.viewToClipMatrix, &cam.proj, sizeof(cam.proj));
	memcpy(commonSettings.worldToViewMatrix, &cam.view, sizeof(cam.proj));

	commonSettings.resolutionScale[0] = 1;
	commonSettings.resolutionScale[1] = 1;
	commonSettings.motionVectorScale[0] = { 0 };
	commonSettings.motionVectorScale[1] = { 0 };
	commonSettings.motionVectorScale[2] = { 0 };
	commonSettings.isMotionVectorInWorldSpace = true;
	commonSettings.denoisingRange = 50000.f;
	commonSettings.isHistoryConfidenceInputsAvailable = false;
	commonSettings.isDisocclusionThresholdMixAvailable = false;
	//commonSettings.accumulationMode = nrd::AccumulationMode::CLEAR_AND_RESTART;
	//commonSettings.enableValidation = true;
	*/
	nrd::ReblurSettings settings = {};
	NRD.SetMethodSettings(nrd::Method::REBLUR_DIFFUSE_SPECULAR, &settings);

	NrdUserPool userPool = {};
	{
		// Fill only required "in-use" inputs and outputs in appropriate slots using entryDescs & entryFormat,
		// applying remapping if necessary. Unused slots will be {nullptr, nri::Format::UNKNOWN}
		
		NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_MV,
			{ &nriTextures[nriTypes::inMV].entryDescs,
			nriTextures[nriTypes::inMV].entryFormat });

		NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_NORMAL_ROUGHNESS,
			{ &nriTextures[nriTypes::inNormalRoughness].entryDescs, 
			nriTextures[nriTypes::inNormalRoughness].entryFormat });

		NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_VIEWZ,
			{ &nriTextures[nriTypes::inViewZ].entryDescs, 
			nriTextures[nriTypes::inViewZ].entryFormat });

		NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST,
			{ &nriTextures[nriTypes::inIndirectDiffuse].entryDescs, 
			nriTextures[nriTypes::inIndirectDiffuse].entryFormat });

		NrdIntegration_SetResource(userPool, nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST,
			{ &nriTextures[nriTypes::outIndirectDiffuse].entryDescs, 
			nriTextures[nriTypes::outIndirectDiffuse].entryFormat });

		NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST,
			{ &nriTextures[nriTypes::inIndirectSpecular].entryDescs,
			nriTextures[nriTypes::inIndirectSpecular].entryFormat });


		NrdIntegration_SetResource(userPool, nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST,
			{ &nriTextures[nriTypes::outIndirectSpecular].entryDescs, 
			nriTextures[nriTypes::outIndirectSpecular].entryFormat });
		
	};

	NRD.Denoise(currentBackbufferIndex, *nriCommandBuffer, commonSettings, userPool, false);
}
