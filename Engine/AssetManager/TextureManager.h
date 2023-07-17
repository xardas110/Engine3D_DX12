#pragma once
#include <Texture.h>
#include <optional>

class SRVHeapData;

struct TextureManager
{
	friend class AssetManager;
    friend class DeferredRenderer;
	friend struct MaterialManager;
    friend struct TextureInstance;

    using SRVHeapID = UINT;
    using TextureID = UINT;
    using TextureRefCount = UINT;

    TextureManager(const SRVHeapData& srvHeapData);

public:

    const TextureInstance& LoadTexture(const std::wstring& path);
    const Texture* GetTexture(TextureInstance textureInstance) const;

private:

    const TextureInstance& CreateTexture(const std::wstring& path);

    void IncreaseRefCount(TextureID textureID);
    void DecreaseRefCount(TextureID textureID);

    struct TextureRegistry
    {
        std::unordered_map<std::wstring, TextureInstance> textureInstanceMap;
        std::vector<Texture> textures;
        std::vector<SRVHeapID> heapIds;
        std::vector<TextureRefCount> refCounts;
    } textureRegistry;

    const SRVHeapData& m_SrvHeapData;    
};
