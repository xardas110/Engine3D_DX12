#include <EnginePCH.h>
#include <TextureManager.h>
#include <Application.h>
#include <CommandList.h>
#include <CommandQueue.h>
#include <AssetManager.h>
#include <Logger.h>
#include <StaticDescriptorHeap.h>

TextureManager::TextureManager(const SRVHeapData& srvHeapData)
    :m_SrvHeapData(srvHeapData) {}

const TextureInstance& TextureManager::CreateTexture(const std::wstring& path)
{
    Texture texture{};
    TextureGPUHandle textureGPUHandle{};
    TextureRefCount textureRefCount{};

    auto device = Application::Get().GetDevice();
    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->GetCommandList();

    commandList->LoadTextureFromFile(texture, path);
    commandQueue->WaitForFenceValue(commandQueue->ExecuteCommandList(commandList));

    if (!texture.IsValid())
    {
       throw std::exception("Failed to create texture resource.");
    }

    device->CreateShaderResourceView(
        texture.GetD3D12Resource().Get(), nullptr,
        m_SrvHeapData.IncrementHandle(textureGPUHandle));

    // If everything succeeded, insert the texture in the textures list and update the map.
    if (!releasedTextureIDs.empty())
    {
        const TextureID textureID = releasedTextureIDs.front();
        releasedTextureIDs.erase(releasedTextureIDs.begin());

        textureRegistry.textureInstanceMap[path].textureID = (TextureID)textureID;
        textureRegistry.textures[textureID] = std::move(texture);
        textureRegistry.refCounts[textureID] = textureRefCount;
        textureRegistry.gpuHandles[textureID] = textureGPUHandle;
    }
    else
    { 
        textureRegistry.textureInstanceMap[path].textureID = (TextureID)textureRegistry.textures.size();
        textureRegistry.textures.emplace_back(std::move(texture));
        textureRegistry.refCounts.emplace_back(textureRefCount);
        textureRegistry.gpuHandles.emplace_back(textureGPUHandle);
    }

    assert((textureRegistry.textures.size() == textureRegistry.refCounts.size() &&
        textureRegistry.textures.size() == textureRegistry.gpuHandles.size()) &&
        "Texture Registry data should have a 1-to-1 relationship.");

    assert((textureRefCount == 0U) && "First texture reference count should always be zero.");

    // Return the instance
    return textureRegistry.textureInstanceMap[path];
}

const TextureInstance& TextureManager::LoadTexture(const std::wstring& path)
{
    auto it = textureRegistry.textureInstanceMap.find(path);
    if (it != textureRegistry.textureInstanceMap.end())
    {
        return it->second;
    }

    return CreateTexture(path);
}

const Texture* TextureManager::GetTexture(const TextureInstance& textureInstance) const
{
    if (!IsTextureInstanceValid(textureInstance)) return nullptr;
    return &textureRegistry.textures[textureInstance.textureID];
}

const std::optional<TextureGPUHandle> TextureManager::GetTextureGPUHandle(const TextureInstance& textureInstance) const
{
    if (!IsTextureInstanceValid(textureInstance)) return std::nullopt;
    return textureRegistry.gpuHandles[textureInstance.textureID];
}

const std::optional<TextureRefCount> TextureManager::GetTextureRefCount(const TextureInstance& textureInstance) const
{
    if (!IsTextureInstanceValid(textureInstance)) return std::nullopt;
    return textureRegistry.refCounts[textureInstance.textureID];
}

bool TextureManager::IsTextureInstanceValid(const TextureInstance& textureInstance) const
{
    if (textureInstance.textureID == TEXTURE_INVALID) return false;
    if (textureInstance.textureID >= textureRegistry.textures.size()) return false;
    if (!textureRegistry.textures[textureInstance.textureID].IsValid()) return false;

    return true;
}

const std::vector<Texture>& TextureManager::GetTextures() const
{
    return textureRegistry.textures;
}

const std::vector<TextureGPUHandle>& TextureManager::GetTextureGPUHandles() const
{
    return textureRegistry.gpuHandles;
}

void TextureManager::IncreaseRefCount(const TextureID textureID)
{
    if (textureID < textureRegistry.refCounts.size())
    {
        textureRegistry.refCounts[textureID]++;
    }
}

void ReleaseTexture(const TextureID textureID, TextureRegistry& textureRegistry)
{
    LOG_INFO("Releasing texture with id %i", (int)textureID);

    textureRegistry.textures[textureID] = Texture();
    textureRegistry.refCounts[textureID] = 0U;
    textureRegistry.gpuHandles[textureID] = 0U;

    //Delete from Texture Instance Map
    for (auto it = textureRegistry.textureInstanceMap.begin(); it != textureRegistry.textureInstanceMap.end(); )
    {
        if (it->second.textureID == textureID)
        {
            it = textureRegistry.textureInstanceMap.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void TextureManager::DecreaseRefCount(const TextureID textureID)
{
    if (textureID < textureRegistry.textures.size() && textureRegistry.refCounts[textureID] > 0)
    {
        if (--textureRegistry.refCounts[textureID] == 0)
        {           
            ReleaseTexture(textureID, textureRegistry);
            releasedTextureIDs.emplace_back(textureID);
        }
    }
}
