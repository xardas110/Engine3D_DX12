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

    
    UNIQUE_LOCK(Instance, textureRegistry.textureInstanceMapMutex);
    UNIQUE_LOCK(ReleasedTextureIDs, releasedTextureIDsMutex);
    UNIQUE_LOCK(Textures, textureRegistry.texturesMutex);
    UNIQUE_LOCK(Refs, textureRegistry.refCountsMutex);
    UNIQUE_LOCK(GPUHandles, textureRegistry.gpuHandlesMutex);

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

    UNIQUE_UNLOCK(ReleasedTextureIDs);
    UNIQUE_UNLOCK(Textures);
    UNIQUE_UNLOCK(Refs);
    UNIQUE_UNLOCK(GPUHandles);

    return textureRegistry.textureInstanceMap[path];
}

const TextureInstance& TextureManager::LoadTexture(const std::wstring& path)
{
    SHARED_LOCK(TextureInstanceMap, textureRegistry.textureInstanceMapMutex);
    auto it = textureRegistry.textureInstanceMap.find(path);
    UNIQUE_UNLOCK(TextureInstanceMap);

    if (it != textureRegistry.textureInstanceMap.end())
    {
        return it->second;
    }

    return CreateTexture(path);
}

const Texture* TextureManager::GetTexture(const TextureInstance& textureInstance) const
{
    if (!IsTextureInstanceValid(textureInstance))
        return nullptr;

    SHARED_LOCK(Textures, textureRegistry.texturesMutex);
    return &textureRegistry.textures[textureInstance.textureID];
}

const std::optional<TextureGPUHandle> TextureManager::GetTextureGPUHandle(const TextureInstance& textureInstance) const
{
    if (!IsTextureInstanceValid(textureInstance))
        return std::nullopt;

    SHARED_LOCK(GPUHandles, textureRegistry.gpuHandlesMutex);
    return textureRegistry.gpuHandles[textureInstance.textureID];
}

const std::optional<TextureRefCount> TextureManager::GetTextureRefCount(const TextureInstance& textureInstance) const
{
    if (!IsTextureInstanceValid(textureInstance))
        return std::nullopt;

    SHARED_LOCK(RefCounts, textureRegistry.refCountsMutex);
    return textureRegistry.refCounts[textureInstance.textureID];
}

bool TextureManager::IsTextureInstanceValid(const TextureInstance& textureInstance) const
{
    SHARED_LOCK(Textures, textureRegistry.texturesMutex);

    if (textureInstance.textureID == TEXTURE_INVALID || textureInstance.textureID >= textureRegistry.textures.size())
        return false;

    return textureRegistry.textures[textureInstance.textureID].IsValid();
}

void TextureManager::IncreaseRefCount(const TextureID textureID)
{
    UNIQUE_LOCK(Refs, textureRegistry.refCountsMutex);
    if (textureID < textureRegistry.refCounts.size())
    {
        textureRegistry.refCounts[textureID]++;
    }
}

void TextureManager::DecreaseRefCount(const TextureID textureID)
{
    {
        UNIQUE_LOCK(Refs, textureRegistry.refCountsMutex);
        if (textureID >= textureRegistry.textures.size() || textureRegistry.refCounts[textureID] <= 0)
            return;

        if (--textureRegistry.refCounts[textureID] != 0)
            return;
    }

    LOG_INFO("Releasing texture with id %i", (int)textureID);

    // Release the texture
    {
        UNIQUE_LOCK(Textures, textureRegistry.texturesMutex);
        textureRegistry.textures[textureID] = Texture();
    }

    {
        UNIQUE_LOCK(GPUHandles, textureRegistry.gpuHandlesMutex);
        textureRegistry.gpuHandles[textureID] = 0U;
    }

    // Delete from Texture Instance Map
    {
        UNIQUE_LOCK(Instance, textureRegistry.textureInstanceMapMutex);
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

    releasedTextureIDs.emplace_back(textureID);
}