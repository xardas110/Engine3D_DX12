#include <EnginePCH.h>
#include <TextureManager.h>
#include <Application.h>
#include <CommandList.h>
#include <CommandQueue.h>
#include <AssetManager.h>
#include <Logger.h>

TextureManager::TextureManager(const SRVHeapData& srvHeapData)
    :m_SrvHeapData(srvHeapData) {}

const TextureInstance& TextureManager::CreateTexture(const std::wstring& path)
{
    auto device = Application::Get().GetDevice();
    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->GetCommandList();

    TextureTuple textureTuple;

    commandList->LoadTextureFromFile(textureTuple.texture, path);
    commandQueue->WaitForFenceValue(commandQueue->ExecuteCommandList(commandList));

    if (!textureTuple.texture.IsValid())
    {
       throw std::exception("Failed to create texture resource.");
    }

    // Create the Shader Resource View. If it fails, return an empty optional.
    device->CreateShaderResourceView(
        textureTuple.texture.GetD3D12Resource().Get(), nullptr,
        m_SrvHeapData.IncrementHandle(textureTuple.heapID));

    // If everything succeeded, insert the texture in the textures list and update the map.
    textureData.textureMap[path].textureID = (TextureID)textureData.textures.size();
    textureData.textures.emplace_back(std::move(textureTuple));

    // Return the instance
    return textureData.textureMap[path];
}

const TextureInstance& TextureManager::LoadTexture(const std::wstring& path)
{
    auto it = textureData.textureMap.find(path);
    if (it != textureData.textureMap.end())
    {
        return it->second;
    }

    return CreateTexture(path);
}

const Texture* TextureManager::GetTexture(TextureInstance textureInstance) const
{
    if (!textureInstance.IsValid()) return nullptr;
    return &textureData.textures[textureInstance.textureID].texture;
}

void TextureManager::IncreaseRefCount(TextureID textureID)
{
    if (textureID < textureData.textures.size())
    {
        textureData.textures[textureID].refCount++;
    }
}

void TextureManager::DecreaseRefCount(TextureID textureID)
{
    if (textureID < textureData.textures.size() && textureData.textures[textureID].refCount > 0)
    {
        if (--textureData.textures[textureID].refCount == 0)
        {
            LOG_WARNING("Removing texture with id %i", (int)textureID);
            textureData.textures[textureID].texture = Texture();
        }
    }
}
