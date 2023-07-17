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

    {
        std::unique_lock<std::shared_mutex> lockReleasedTextureIDs(releasedTextureIDsMutex);
        std::unique_lock<std::shared_mutex> lockInstance(textureRegistry.textureInstanceMapMutex);
        std::unique_lock<std::shared_mutex> lockTextures(textureRegistry.texturesMutex);
        std::unique_lock<std::shared_mutex> lockRefs(textureRegistry.refCountsMutex);
        std::unique_lock<std::shared_mutex> lockGPUHandles(textureRegistry.gpuHandlesMutex);

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

        lockGPUHandles.unlock();
        lockRefs.unlock();
        lockTextures.unlock();
        lockInstance.unlock();
        lockReleasedTextureIDs.unlock();
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
    std::shared_lock<std::shared_mutex> lock(textureRegistry.textureInstanceMapMutex);
    auto it = textureRegistry.textureInstanceMap.find(path);
    lock.unlock(); // we don't need the lock beyond this point

    if (it != textureRegistry.textureInstanceMap.end())
    {
        return it->second;
    }

    return CreateTexture(path);
}

const Texture* TextureManager::GetTexture(const TextureInstance& textureInstance) const
{
    if (!IsTextureInstanceValid(textureInstance)) return nullptr;

    std::shared_lock<std::shared_mutex> lockTextures(textureRegistry.texturesMutex);
    return &textureRegistry.textures[textureInstance.textureID];
}

const std::optional<TextureGPUHandle> TextureManager::GetTextureGPUHandle(const TextureInstance& textureInstance) const
{
    if (!IsTextureInstanceValid(textureInstance)) return std::nullopt;

    std::shared_lock<std::shared_mutex> lockHandles(textureRegistry.gpuHandlesMutex);
    return textureRegistry.gpuHandles[textureInstance.textureID];
}

const std::optional<TextureRefCount> TextureManager::GetTextureRefCount(const TextureInstance& textureInstance) const
{
    if (!IsTextureInstanceValid(textureInstance)) return std::nullopt;

    std::shared_lock<std::shared_mutex> lockRefCounts(textureRegistry.refCountsMutex);
    return textureRegistry.refCounts[textureInstance.textureID];
}

bool TextureManager::IsTextureInstanceValid(const TextureInstance& textureInstance) const
{
    std::shared_lock<std::shared_mutex> lock(textureRegistry.texturesMutex);

    if (textureInstance.textureID == TEXTURE_INVALID || textureInstance.textureID >= textureRegistry.textures.size())
        return false;

    return textureRegistry.textures[textureInstance.textureID].IsValid();
}

const std::vector<Texture>& TextureManager::GetTexturesNotThreadSafe() const
{
    std::shared_lock<std::shared_mutex> lock(textureRegistry.texturesMutex);
    return textureRegistry.textures;
}

const std::vector<Texture> TextureManager::GetTexturesThreadSafe() const
{
    std::shared_lock<std::shared_mutex> lock(textureRegistry.texturesMutex);
    return textureRegistry.textures;
}

const std::vector<TextureGPUHandle>& TextureManager::GetTextureGPUHandlesNotThreadSafe() const
{
    std::shared_lock<std::shared_mutex> lock(textureRegistry.gpuHandlesMutex);
    return textureRegistry.gpuHandles;
}


const std::vector<TextureGPUHandle> TextureManager::GetTextureGPUHandlesThreadSafe() const
{
    std::shared_lock<std::shared_mutex> lock(textureRegistry.gpuHandlesMutex);
    return textureRegistry.gpuHandles;
}

void TextureManager::IncreaseRefCount(const TextureID textureID)
{
    std::unique_lock lock(textureRegistry.refCountsMutex);
    if (textureID < textureRegistry.refCounts.size())
    {
        textureRegistry.refCounts[textureID]++;
    }
}

void TextureManager::DecreaseRefCount(const TextureID textureID)
{
    {
        std::unique_lock lock(textureRegistry.refCountsMutex);
        if (textureID >= textureRegistry.textures.size() || textureRegistry.refCounts[textureID] <= 0)
            return;

        if (--textureRegistry.refCounts[textureID] != 0)
            return;
    }

    LOG_INFO("Releasing texture with id %i", (int)textureID);

    // Release the texture
    {
        std::unique_lock lock(textureRegistry.texturesMutex);
        textureRegistry.textures[textureID] = Texture();
    }

    {
        std::unique_lock lock(textureRegistry.gpuHandlesMutex);
        textureRegistry.gpuHandles[textureID] = 0U;
    }

    //Delete from Texture Instance Map
    {
        std::unique_lock lock(textureRegistry.textureInstanceMapMutex);
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
