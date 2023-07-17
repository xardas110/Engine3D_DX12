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
	: m_TextureManager(m_SrvHeapData), m_MaterialManager(m_TextureManager), m_MeshManager(m_SrvHeapData)
{}

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

const TextureManager& AssetManager::GetTextureManager() const
{
	return m_TextureManager;
}

MaterialManager& AssetManager::GetMaterialManager()
{
	return m_MaterialManager;
}

const MeshManager& AssetManager::GetMeshManager() const
{
	return m_MeshManager;
}
