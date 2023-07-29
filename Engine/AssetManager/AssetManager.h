#pragma once
#include "Mesh.h"
#include <Texture.h>
#include <Material.h>
#include <entt/entt.hpp>
#include <MeshManager.h>
#include <MaterialManager.h>
#include <TextureManager.h>
#include <StaticDescriptorHeap.h>
#include <event.hpp>

class AssetManager
{
	friend class Application;
	friend class std::default_delete<AssetManager>;

private:
	AssetManager();
	~AssetManager();

	AssetManager(const AssetManager& other) = delete;
	AssetManager& operator=(const AssetManager& other) = delete;

	AssetManager(AssetManager&& other) = delete;
	AssetManager& operator=(AssetManager&&) = delete;

	static std::unique_ptr<AssetManager>	CreateInstance();

public:
	const SRVHeapData&		GetSRVHeapData() const;

	std::unique_ptr<TextureManager>			m_TextureManager;
	std::unique_ptr<MaterialManager>		m_MaterialManager;
	std::unique_ptr<MeshManager>			m_MeshManager;
private:
	SRVHeapData								m_SrvHeapData;
};