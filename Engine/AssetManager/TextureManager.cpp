#include <EnginePCH.h>
#include <TextureManager.h>
#include <Application.h>
#include <CommandList.h>
#include <CommandQueue.h>
#include <AssetManager.h>
#include <Logger.h>
#include <StaticDescriptorHeap.h>

std::unique_ptr<TextureManager> TextureManagerAccess::CreateTextureManager(const SRVHeapData& srvHeapData)
{
    return std::unique_ptr<TextureManager>(new TextureManager(srvHeapData));
}

TextureManager::TextureManager(const SRVHeapData& srvHeapData)
    :m_SrvHeapData(srvHeapData) {}

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
        LOG_ERROR("Failed to create texture resource: %s", std::string(path.begin(), path.end()).c_str());
        return TextureInstance();
    }

    device->CreateShaderResourceView(
        texture.GetD3D12Resource().Get(), nullptr,
        m_SrvHeapData.IncrementHandle(textureGPUHandle));

    
    SCOPED_UNIQUE_LOCK(
        textureRegistry.textureInstanceMapMutex,
        releasedTextureIDsMutex,
        textureRegistry.texturesMutex,
        textureRegistry.gpuHandlesMutex
    );

    // If everything succeeded, insert the texture in the textures list and update the map.
    TextureID textureID = !releasedTextureIDs.empty() ? 
        releasedTextureIDs.front() : (TextureID)textureRegistry.textures.size();

    if (textureID >= TEXTURE_MANAGER_MAX_TEXTURES)
    {
        throw std::runtime_error("TextureID exceeds the maximum limit");
    }

    textureRegistry.textureInstanceMap[path].textureID = textureID;
    if (!releasedTextureIDs.empty()) 
    {
        releasedTextureIDs.erase(releasedTextureIDs.begin());
        textureRegistry.textures[textureID] = std::move(texture);
        textureRegistry.gpuHandles[textureID] = textureGPUHandle;
    }
    else 
    {
        textureRegistry.textures.emplace_back(std::move(texture));
        textureRegistry.gpuHandles.emplace_back(textureGPUHandle);
    }
    
    textureRegistry.refCounts[textureID].store(textureRefCount);

    // Send a event that a texture was created.
    textureInstanceCreatedEvent(textureRegistry.textureInstanceMap[path]);

    return textureRegistry.textureInstanceMap[path];
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

    return textureRegistry.refCounts[textureInstance.textureID].load();
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
    if (textureID < textureRegistry.refCounts.size())
    {
        textureRegistry.refCounts[textureID].fetch_add(1);
    }
}

void TextureManager::DecreaseRefCount(const TextureID textureID)
{
    if (textureID >= textureRegistry.textures.size())
        return;

    if (textureRegistry.refCounts[textureID].fetch_sub(1) != 1)
        return;

    ReleaseTexture(textureID);
}

void TextureManager::ReleaseTexture(const TextureID textureID)
{
    // Ensure textureID is valid before attempting to release
    if (textureID >= textureRegistry.textures.size())
        return;

    LOG_INFO("Releasing texture with id %i", (UINT)textureID);

    // Release the texture
    {
        UNIQUE_LOCK(Textures, textureRegistry.texturesMutex);
        textureRegistry.textures[textureID] = Texture();
        assert(!textureRegistry.textures[textureID].IsValid());
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

    // Add to list of released textures
    UNIQUE_LOCK(ReleasedTextureIDs, releasedTextureIDsMutex);
    releasedTextureIDs.emplace_back(textureID);

    // Send event that the texture was deleted.
    textureInstanceDeletedEvent(textureID);
}
