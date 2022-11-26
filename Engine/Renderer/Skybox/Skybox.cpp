#include <EnginePCH.h>
#include <Skybox/Skybox.h>
#include <Application.h>
#include <CommandList.h>
#include <CommandQueue.h>

Skybox::Skybox(const std::wstring& panoPath)
	:heap(6)
{
	
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	commandList->LoadTextureFromFile(panoTexture, panoPath);

	auto cubemapDesc = panoTexture.GetD3D12ResourceDesc();
	cubemapDesc.Width = cubemapDesc.Height = 1024;
	cubemapDesc.DepthOrArraySize = 6;
	cubemapDesc.MipLevels = 0;

	cubemap = Texture(cubemapDesc, nullptr, TextureUsage::Albedo, panoPath);
	commandList->PanoToCubemap(cubemap, panoTexture);
	
	auto fenceVal = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceVal);

	auto device = Application::Get().GetDevice();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = cubemap.GetD3D12ResourceDesc().Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = (UINT)-1; // Use all mips.

	device->CreateShaderResourceView(
		cubemap.GetD3D12Resource().Get(), &srvDesc,
		heap.SetHandle(0));
}


D3D12_GPU_DESCRIPTOR_HANDLE Skybox::GetSRVView()
{
	return heap.GetHandleAtStart();
}