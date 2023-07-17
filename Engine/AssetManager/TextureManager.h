#pragma once
#include <Texture.h>
#include <StaticDescriptorHeap.h>

struct TextureRegistry
{
    std::unordered_map<std::wstring, TextureInstance> textureInstanceMap;
    std::vector<Texture> textures;
    std::vector<TextureRefCount> refCounts;
    std::vector<TextureGPUHandle> gpuHandles;
};

class TextureManager {
    friend class AssetManager;
    friend struct TextureInstance;

public:
    explicit TextureManager(const SRVHeapData& srvHeapData);

    // Public interface for managing textures
    const TextureInstance& LoadTexture(const std::wstring& path);
    bool IsTextureInstanceValid(const TextureInstance& textureInstance) const;
    const std::vector<Texture>& GetTextures() const;
    const std::vector<TextureGPUHandle>& GetTextureGPUHandles() const;
    const Texture* GetTexture(const TextureInstance& textureInstance) const;
    const std::optional<TextureGPUHandle> GetTextureGPUHandle(const TextureInstance& textureInstance) const;
    const std::optional<TextureRefCount> GetTextureRefCount(const TextureInstance& textureInstance) const;

private:
    // Methods for internal use
    const TextureInstance& CreateTexture(const std::wstring& path);
    void IncreaseRefCount(const TextureID textureID);
    void DecreaseRefCount(const TextureID textureID);

    // Member variables
    std::vector<TextureID> releasedTextureIDs;
    TextureRegistry textureRegistry;
    const SRVHeapData& m_SrvHeapData;
};
