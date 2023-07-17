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

    // Create the Shader Resource View. If it fails, return an empty optional.
    device->CreateShaderResourceView(
        texture.GetD3D12Resource().Get(), nullptr,
        m_SrvHeapData.IncrementHandle(textureGPUHandle));

    // If everything succeeded, insert the texture in the textures list and update the map.
    textureRegistry.textureInstanceMap[path].textureID = (TextureID)textureRegistry.textures.size();
    textureRegistry.textures.emplace_back(std::move(texture));
    textureRegistry.refCounts.emplace_back(textureRefCount);
    textureRegistry.gpuHandles.emplace_back(textureGPUHandle);

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

const Texture* TextureManager::GetTexture(TextureInstance textureInstance) const
{
    if (!IsTextureInstanceValid(textureInstance)) return nullptr;
    return &textureRegistry.textures[textureInstance.textureID];
}

const std::optional<TextureGPUHandle> TextureManager::GetTextureGPUHandle(TextureInstance textureInstance) const
{
    if (!IsTextureInstanceValid(textureInstance)) return std::nullopt;
    return textureRegistry.gpuHandles[textureInstance.textureID];
}

const std::optional<TextureRefCount> TextureManager::GetTextureRefCount(TextureInstance textureInstance) const
{
    if (!IsTextureInstanceValid(textureInstance)) return std::nullopt;
    return textureRegistry.refCounts[textureInstance.textureID];
}

bool TextureManager::IsTextureInstanceValid(TextureInstance textureInstance) const
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

void TextureManager::IncreaseRefCount(TextureID textureID)
{
    if (textureID < textureRegistry.refCounts.size())
    {
        textureRegistry.refCounts[textureID]++;
    }
}

void TextureManager::DecreaseRefCount(TextureID textureID)
{
    if (textureID < textureRegistry.textures.size() && textureRegistry.refCounts[textureID] > 0)
    {
        if (--textureRegistry.refCounts[textureID] == 0)
        {
            LOG_WARNING("Removing texture with id %i", (int)textureID);
            textureRegistry.textures[textureID] = Texture();
        }
    }
}
