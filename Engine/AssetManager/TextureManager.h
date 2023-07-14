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
	//Must be valid texture id
	const Texture* GetTexture(TextureID textureID);
private:

	TextureManager(const SRVHeapData& srvHeapData);

	struct TextureTuple
	{
		Texture texture;
		SRVHeapID heapID{ UINT_MAX };
	};

	struct TextureData
	{
		std::map<std::wstring, TextureID> textureMap;
		std::vector<TextureTuple> textures;		
	} textureData;

	TextureID CreateTexture(const std::wstring& path);

	const SRVHeapData& m_SrvHeapData;
};
