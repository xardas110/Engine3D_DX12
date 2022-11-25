#include <EnginePCH.h>
#include <NvidiaDenoiser.h>
#include <Application.h>
#include <CommandQueue.h>

#include <NRIDescs.hpp>
#include <NRDIntegration.hpp>

#include <Helpers.h>
#include <imgui.h>

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

namespace nriTypes
{
	enum Type
	{
		inMV,
		inNormalRoughness,
		inViewZ,
		inIndirectDiffuse,
		inIndirectSpecular,
		outIndirectDiffuse,
		outIndirectSpecular,
		size
	};
}

struct NRITextures
{
	nri::TextureTransitionBarrierDesc entryDescs = {};
	nri::Format entryFormat = {};
}nriTextures[nriTypes::size];

NvidiaDenoiser::NvidiaDenoiser(int width, int height)
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

	NRDERRORCHECK(nriResult);

	nriResult = nri::GetInterface(*m_NRIDevice,
		NRI_INTERFACE(nri::HelperInterface), (nri::HelperInterface*)&NRI);

	NRDERRORCHECK(nriResult);

	// Get appropriate "wrapper" extension (XXX - can be D3D11, D3D12 or VULKAN)
	nriResult = nri::GetInterface(*m_NRIDevice,
		NRI_INTERFACE(nri::WrapperD3D12Interface), (nri::WrapperD3D12Interface*)&NRI);

	NRDERRORCHECK(nriResult);

	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)));

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
}

NvidiaDenoiser::~NvidiaDenoiser()
{
	//Clean this sht somehow

	m_CommandAllocator->Reset();

	NRD.Destroy();

	//NRI.DestroyCommandBuffer(*nriCommandBuffer);

	//for (uint32_t i = 0; i < nriTypes::size; i++)
	//	NRI.DestroyTexture((nri::Texture&)nriTextures[i].entryDescs.texture);
	
	nri::DestroyDevice(*m_NRIDevice);
	
}

void NvidiaDenoiser::RenderFrame(CommandList& commandList, const CameraCB &cam, DenoiserTextures& texs, int currentBackbufferIndex, int width, int height)
{
	/*
	nri::CommandBufferD3D12Desc commandBufferDesc = {};
	commandBufferDesc.d3d12CommandList = (ID3D12GraphicsCommandList*)commandList.GetGraphicsCommandList().Get();
	commandBufferDesc.d3d12CommandAllocator = m_CommandAllocator.Get();

	NRI.CreateCommandBufferD3D12(*m_NRIDevice, commandBufferDesc, nriCommandBuffer);

	NRITextures nriTextures[nriTypes::size];

	{//Normalrough
		nri::TextureD3D12Desc normalRoughDesc = {};

		normalRoughDesc.d3d12Resource = texs.inNormalRough.GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::inNormalRoughness].entryDescs;

		nriTextures[nriTypes::inNormalRoughness].entryFormat =
			nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R10G10B10A2_UNORM);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, normalRoughDesc, (nri::Texture*&)entryDesc.texture));

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};
	{//Motion Vector
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = texs.inMV.GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::inMV].entryDescs;

		nriTextures[nriTypes::inMV].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, (nri::Texture*&)entryDesc.texture));

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};
	{//viewZ
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = texs.inViewZ.GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::inViewZ].entryDescs;

		nriTextures[nriTypes::inViewZ].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R32_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, (nri::Texture*&)entryDesc.texture));

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};
	{//Indirect Diffuse
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = texs.inIndirectDiffuse.GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::inIndirectDiffuse].entryDescs;

		nriTextures[nriTypes::inIndirectDiffuse].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, (nri::Texture*&)entryDesc.texture));

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};
	{//Indirect Specular
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = texs.inIndirectSpecular.GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::inIndirectSpecular].entryDescs;

		nriTextures[nriTypes::inIndirectSpecular].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, (nri::Texture*&)entryDesc.texture));

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
		entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	};
	{//OUT Indirect Diffuse
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = texs.outIndirectDiffuse.GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::outIndirectDiffuse].entryDescs;

		nriTextures[nriTypes::outIndirectDiffuse].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, (nri::Texture*&)entryDesc.texture));

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE_STORAGE;
		entryDesc.nextLayout = nri::TextureLayout::GENERAL;
	};
	{//OUT Indirect Specular
		nri::TextureD3D12Desc texDesc = {};
		texDesc.d3d12Resource = texs.outIndirectSpecular.GetD3D12Resource().Get();

		nri::TextureTransitionBarrierDesc& entryDesc = nriTextures[nriTypes::outIndirectSpecular].entryDescs;

		nriTextures[nriTypes::outIndirectSpecular].entryFormat = nri::ConvertDXGIFormatToNRI(DXGI_FORMAT_R16G16B16A16_FLOAT);

		NRDERRORCHECK(NRI.CreateTextureD3D12(*m_NRIDevice, texDesc, (nri::Texture*&)entryDesc.texture));

		entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE_STORAGE;
		entryDesc.nextLayout = nri::TextureLayout::GENERAL;
	};


	commonSettings = {};

	memcpy(commonSettings.worldToViewMatrix, &cam.view, sizeof(cam.view));
	memcpy(commonSettings.viewToClipMatrix, &cam.proj, sizeof(cam.proj));
	
	memcpy(commonSettings.worldToViewMatrixPrev, &cam.prevView, sizeof(cam.prevView));
	memcpy(commonSettings.viewToClipMatrixPrev, &cam.prevProj, sizeof(cam.prevProj));

	commonSettings.motionVectorScale[0] = { 0 };
	commonSettings.motionVectorScale[1] = { 0 };
	commonSettings.motionVectorScale[2] = { 0 };
	commonSettings.isMotionVectorInWorldSpace = true;

	nrd::ReblurSettings settings = {};
	NRD.SetMethodSettings(nrd::Method::REBLUR_DIFFUSE_SPECULAR, &settings);

	NrdUserPool userPool = {};
	{
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
	}

	NRD.Denoise(currentBackbufferIndex, *nriCommandBuffer, commonSettings, userPool, true);
	*/
}
