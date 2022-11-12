#pragma once
#include "Mesh.h"
#include <Texture.h>

class AssetManager
{
	friend class Application;
	friend class DeferredRenderer;
	friend class std::default_delete<AssetManager>;

	using StaticMeshMap = std::map<std::string, StaticMesh>;
	using TextureMap = std::map<std::wstring, TextureID>;

public:
	bool LoadStaticMesh(const std::string& path, StaticMesh& outStaticMesh);
	bool LoadTexture(const std::wstring& path, TextureID& id);
private:
	AssetManager();
	~AssetManager();

	static std::unique_ptr<AssetManager> CreateInstance();

	StaticMeshMap staticMeshMap;

	struct TextureData
	{
		friend class AssetManager;
		friend class DeferredRenderer;
	
		std::uint32_t Size() const
		{
			return lastIndex;
		}

	private:
		TextureMap textureMap;
		Texture m_Textures[5000];
		std::uint32_t lastIndex = 0;
	} m_TextureData;

	Mesh m_Primitives[Primitives::Size];
	Mesh m_Meshes[50000]; std::uint32_t lastIndex = 0;
};