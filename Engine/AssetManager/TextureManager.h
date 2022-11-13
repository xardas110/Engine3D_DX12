#pragma once
#include <Texture.h>


class SRVHeapData;

struct TextureManager
{
	friend class AssetManager;
	friend class MaterialManager;
	friend class DeferredRenderer;
	using SRVHeapID = UINT;

	bool LoadTexture(const std::wstring& path, TextureInstance& outTextureInstance);

private:

	TextureManager(const SRVHeapData& srvHeapData);

	struct TextureTuple
	{
		Texture texture;
		//Muteable so MeshManager can increment refcount when having const access to this class
		mutable UINT refCounter{0U};
		SRVHeapID heapID{ UINT_MAX };
	};

	struct TextureData
	{
		std::map<std::wstring, TextureID> textureMap;
		std::vector<TextureTuple> textures;		
	} textureData;

	TextureID CreateTexture(const std::wstring& path);

	void IncrementRef(TextureID id) const;
	void DecrementRef(TextureID id) const;

	const SRVHeapData& m_SrvHeapData;
};
