#include <EnginePCH.h>
#include <TextureManager.h>
#include <Application.h>
#include <CommandList.h>
#include <CommandQueue.h>
#include <AssetManager.h>
#include <Logger.h>

TextureManager::TextureManager(const SRVHeapData& srvHeapData)
    :m_SrvHeapData(srvHeapData) {}

std::optional<TextureManager::TextureID> TextureManager::CreateTexture(const std::wstring& path)
{
    auto device = Application::Get().GetDevice();
    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->GetCommandList();

    TextureTuple textureTuple;

    commandList->LoadTextureFromFile(textureTuple.texture, path);
    commandQueue->WaitForFenceValue(commandQueue->ExecuteCommandList(commandList));

    // Create the Shader Resource View. If it fails, return an empty optional.
    device->CreateShaderResourceView(
        textureTuple.texture.GetD3D12Resource().Get(), nullptr,
        m_SrvHeapData.IncrementHandle(textureTuple.heapID));

    // If everything succeeded, insert the texture in the textures list and update the map.
    const auto currentIndex = textureData.textures.size();
    textureData.textureMap[path] = currentIndex;
    textureData.textures.emplace_back(std::move(textureTuple));

    // Return the index to the new texture
    return currentIndex;
}

bool TextureManager::LoadTexture(const std::wstring& path, TextureInstance& outTextureInstance)
{
    auto it = textureData.textureMap.find(path);
    if (it != textureData.textureMap.end())
    {
        outTextureInstance.textureID = it->second;
        return true;
    }

    std::optional<TextureID> textureID = CreateTexture(path);
    if (!textureID.has_value()) // Failed to create texture
        return false;

    outTextureInstance.textureID = textureID.value();
    IncreaseRefCount(textureID.value());
    return true;
}

const Texture* TextureManager::GetTexture(TextureID textureID) const
{
    if (textureID >= textureData.textures.size()) return nullptr;
    return &textureData.textures[textureID].texture;
}

void TextureManager::IncreaseRefCount(TextureID textureID)
{
    if (textureID < textureData.textures.size())
    {
        textureData.textures[textureID].refCount++;
    }
    else
    {
        throw std::out_of_range("Invalid texture ID.");
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
    else
    {
        throw std::out_of_range("Invalid texture ID or the texture was not referenced.");
    }
}
