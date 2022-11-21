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

NrdIntegration NRD = NrdIntegration(3);

struct NriInterface
	: public nri::CoreInterface
	, public nri::HelperInterface
	, public nri::WrapperD3D12Interface
{};

nri::Device* m_NRIDevice;

NriInterface NRI;

NvidiaDenoiser::NvidiaDenoiser()
{
	auto device = Application::Get().GetDevice();
	auto cq = Application::Get().GetCommandQueue();

	m_Adapter = Application::Get().GetAdapter(false).Get();

	nri::DeviceCreationD3D12Desc deviceDesc = {};
	deviceDesc.d3d12Device = device.Get();

	deviceDesc.d3d12PhysicalAdapter = m_Adapter.Get();
	deviceDesc.d3d12GraphicsQueue = cq->GetD3D12CommandQueue().Get();
	deviceDesc.enableNRIValidation = false;
	
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

}

NvidiaDenoiser::~NvidiaDenoiser()
{

	// Also NRD needs to be recreated on "resize"
	//NRD.Destroy();
}

void NvidiaDenoiser::Init(int width, int height, const Texture& diffuseSpecular)
{
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

	auto cq = Application::Get().GetCommandQueue();

	nri::CommandBufferD3D12Desc commandBufferDesc = {};
	commandBufferDesc.d3d12CommandList = (ID3D12GraphicsCommandList*)cq->GetCommandList()->GetGraphicsCommandList().Get();

	nri::CommandBuffer* nriCommandBuffer = nullptr;
	NRI.CreateCommandBufferD3D12(*m_NRIDevice, commandBufferDesc, nriCommandBuffer);

	nri::TextureTransitionBarrierDesc entryDescs[1] = {};
	nri::Format entryFormat[1] = {};

	nri::TextureTransitionBarrierDesc& entryDesc = entryDescs[0];

	nri::TextureD3D12Desc textureDesc = {};
	textureDesc.d3d12Resource = diffuseSpecular.GetD3D12Resource().Get();
	NRI.CreateTextureD3D12(*m_NRIDevice, textureDesc, (nri::Texture*&)entryDesc.texture);

	// You need to specify the current state of the resource here, after denoising NRD can modify
	// this state. Application must continue state tracking from this point.
	// Useful information:
	//    SRV = nri::AccessBits::SHADER_RESOURCE, nri::TextureLayout::SHADER_RESOURCE
	//    UAV = nri::AccessBits::SHADER_RESOURCE_STORAGE, nri::TextureLayout::GENERAL
	entryDesc.nextAccess = nri::AccessBits::SHADER_RESOURCE;
	entryDesc.nextLayout = nri::TextureLayout::SHADER_RESOURCE;
	
	nrd::CommonSettings commonSettings = {};

	nrd::ReblurSettings settings = {};
	NRD.SetMethodSettings(nrd::Method::REBLUR_DIFFUSE_SPECULAR, &settings);

	NrdUserPool userPool = {};

	{
		NrdIntegrationTexture tex;
		tex.format = nri::Format::RGBA32_SFLOAT;
		// Fill only required "in-use" inputs and outputs in appropriate slots using entryDescs & entryFormat,
		// applying remapping if necessary. Unused slots will be {nullptr, nri::Format::UNKNOWN}
		NrdIntegration_SetResource(userPool, nrd::ResourceType::IN_NORMAL_ROUGHNESS, tex);

	};

	// Better use "true" if resources are not changing between frames (i.e. are not suballocated from a heap)
	bool enableDescriptorCaching = true;

	NRD.Denoise(0, *nriCommandBuffer, commonSettings, userPool, enableDescriptorCaching);
}

