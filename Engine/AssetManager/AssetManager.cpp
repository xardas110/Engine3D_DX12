#include <EnginePCH.h>
#include <AssetManager.h>
#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <AssimpLoader.h>


SRVHeapData::SRVHeapData()
	: increment(Application::Get().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
{
	auto device = Application::Get().GetDevice();
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = 1024;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
}

std::wstring StringToWString(const std::string& s)
{
	return std::wstring(s.begin(), s.end());
}

AssetManager::AssetManager()
{

}

AssetManager::~AssetManager()
{
	//m_Primitives will be deleted once its unreferenced.
}

std::unique_ptr<AssetManager> AssetManager::CreateInstance()
{
	return std::unique_ptr<AssetManager>(new AssetManager);
}
