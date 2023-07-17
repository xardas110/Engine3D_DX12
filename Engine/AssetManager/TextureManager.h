#pragma once
#include <Texture.h>
#include <StaticDescriptorHeap.h>

struct TextureManager
{
	friend class AssetManager;
    friend struct TextureInstance;

private:

    TextureManager(const SRVHeapData& srvHeapData);

public:

    const TextureInstance& LoadTexture(const std::wstring& path);
    const Texture* GetTexture(const TextureInstance& textureInstance) const;
    const std::optional<TextureGPUHandle> GetTextureGPUHandle(const TextureInstance& textureInstance) const;
    const std::optional<TextureRefCount> GetTextureRefCount(const TextureInstance& textureInstance) const;

    bool IsTextureInstanceValid(const TextureInstance& textureInstance) const;

    const std::vector<Texture>& GetTextures() const;
    const std::vector<TextureGPUHandle>& GetTextureGPUHandles() const;
private:

    const TextureInstance& CreateTexture(const std::wstring& path);

    void IncreaseRefCount(const TextureID textureID);
    void DecreaseRefCount(const TextureID textureID);

    struct TextureRegistry
    {
        std::unordered_map<std::wstring, TextureInstance> textureInstanceMap;
        std::vector<Texture> textures;        
        std::vector<TextureRefCount> refCounts;
        std::vector<TextureGPUHandle> gpuHandles;
    } textureRegistry;

    const SRVHeapData& m_SrvHeapData;    
};
