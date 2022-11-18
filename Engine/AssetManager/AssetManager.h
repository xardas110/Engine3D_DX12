#pragma once
#include "Mesh.h"
#include <Texture.h>
#include <Material.h>
#include <entt/entt.hpp>
#include <MeshManager.h>
#include <MaterialManager.h>
#include <TextureManager.h>
#include <StaticDescriptorHeap.h>

using RefCounter = UINT;

class AssetManager
{
	friend class Application;
	friend class TextureInstance;
	friend class MaterialInstance;
	friend class MeshInstance;
	friend class DeferredRenderer;
	friend class std::default_delete<AssetManager>;

private:
	AssetManager();
	~AssetManager();

	static std::unique_ptr<AssetManager> CreateInstance();

public:
	//Do not change order! SRVHeapData must be created first -> texturemanager
	SRVHeapData m_SrvHeapData;
	TextureManager m_TextureManager;
	MaterialManager m_MaterialManager;
	MeshManager m_MeshManager;	
};