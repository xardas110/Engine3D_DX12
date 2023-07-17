#pragma once
#include <Texture.h>
#include <optional>

class SRVHeapData;

struct TextureManager
{
	friend class AssetManager;
	friend class MaterialManager;
	friend class DeferredRenderer;
    friend struct TextureInstance;

    using SRVHeapID = UINT;
    using TextureID = UINT;

    TextureManager(const SRVHeapData& srvHeapData);

public:

    const TextureInstance& LoadTexture(const std::wstring& path);

    const Texture* GetTexture(TextureInstance textureInstance) const;

private:

    void IncreaseRefCount(TextureID textureID);
    void DecreaseRefCount(TextureID textureID);

    const TextureInstance& CreateTexture(const std::wstring& path);

    struct TextureTuple
    {
        Texture texture;
        SRVHeapID heapID{ UINT_MAX };
        UINT refCount{ 0 };
    };

    struct TextureData
    {
        std::unordered_map<std::wstring, TextureInstance> textureMap;
        std::vector<TextureTuple> textures;
    } textureData;

    const SRVHeapData& m_SrvHeapData;    
};
