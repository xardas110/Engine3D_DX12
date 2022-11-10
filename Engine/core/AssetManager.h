#pragma once
#include "Mesh.h"

namespace Primitives
{
	enum
	{
		Cube,
		Size
	};
}

class AssetManager
{
	friend class Application;
	friend class DeferredRenderer;
	friend class std::default_delete<AssetManager>;

	using StaticMeshMap = std::map<std::string, StaticMesh>;

public:
	bool LoadStaticMesh(const std::string& path, StaticMesh& outStaticMesh);
	//bool LoadAudiofile(const std::string& path, Audioid)
private:
	AssetManager();
	~AssetManager();

	static std::unique_ptr<AssetManager> CreateInstance();

	StaticMeshMap staticMeshMap;
	Mesh m_Primitives[Primitives::Size];
	Mesh m_Meshes[50000]; std::uint32_t lastIndex = 0;
};