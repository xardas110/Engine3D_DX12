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

public:
    using SRVHeapID = UINT;
    using TextureID = size_t; // Assuming TextureID is size_t

    TextureManager(const SRVHeapData& srvHeapData);

    bool LoadTexture(const std::wstring& path, TextureInstance& outTextureInstance);

    const Texture* GetTexture(TextureID textureID) const;

private:

    void IncreaseRefCount(TextureID textureID);
    void DecreaseRefCount(TextureID textureID);

    struct TextureTuple
    {
        Texture texture;
        SRVHeapID heapID{ UINT_MAX };
        UINT refCount{ 0 };
    };

    struct TextureData
    {
        std::unordered_map<std::wstring, TextureID> textureMap;
        std::vector<TextureTuple> textures;
    } textureData;

    const SRVHeapData& m_SrvHeapData;

    std::optional<TextureID> CreateTexture(const std::wstring& path);
};
