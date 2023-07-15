#include <EnginePCH.h>
#include <TextureManager.h>
#include <Application.h>
#include <CommandList.h>
#include <CommandQueue.h>
#include <AssetManager.h>

TextureManager::TextureManager(const SRVHeapData& srvHeapData)
	:m_SrvHeapData(srvHeapData)
{}

bool TextureManager::LoadTexture(const std::wstring& path, TextureInstance& outTextureInstance)
{
	if (textureData.textureMap.find(path) != textureData.textureMap.end())
	{
		outTextureInstance.textureID = textureData.textureMap[path];
		return true;
	}

	outTextureInstance.textureID = CreateTexture(path);
	return true;
}

const Texture* TextureManager::GetTexture(TextureID textureID)
{
	if (textureID == UINT_MAX) return nullptr;

	return &textureData.textures[textureID].texture;
}

TextureID TextureManager::CreateTexture(const std::wstring& path)
{
	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	TextureTuple textureTuple;

	commandList->LoadTextureFromFile(textureTuple.texture, path);
	commandQueue->WaitForFenceValue(commandQueue->ExecuteCommandList(commandList));

	device->CreateShaderResourceView(
		textureTuple.texture.GetD3D12Resource().Get(), nullptr, 
		m_SrvHeapData.IncrementHandle(textureTuple.heapID));

	const auto currentIndex = textureData.textures.size();
	textureData.textureMap[path] = currentIndex;
	textureData.textures.emplace_back(std::move(textureTuple));

	return currentIndex;
}
