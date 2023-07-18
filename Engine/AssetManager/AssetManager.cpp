#include <EnginePCH.h>
#include <AssetManager.h>
#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <AssimpLoader.h>

std::wstring StringToWString(const std::string& s)
{
	return std::wstring(s.begin(), s.end());
}

AssetManager::AssetManager() 
{
	m_TextureManager = TextureManagerAccess::CreateTextureManager(m_SrvHeapData);
	m_MaterialManager = std::unique_ptr<MaterialManager>(new MaterialManager(*m_TextureManager.get()));
	m_MeshManager = std::unique_ptr<MeshManager>(new MeshManager(m_SrvHeapData));
}

AssetManager::~AssetManager()
{}

std::unique_ptr<AssetManager> AssetManager::CreateInstance()
{
	return std::unique_ptr<AssetManager>(new AssetManager);
}

const SRVHeapData& AssetManager::GetSRVHeapData() const
{
	return m_SrvHeapData;
}
