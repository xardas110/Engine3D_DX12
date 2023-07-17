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
	friend class TextureInstance;
	friend class MaterialInstance;
	friend class MeshInstance;
	friend class DeferredRenderer;

	friend struct StaticMeshInstance;

	friend class std::default_delete<AssetManager>;

private:
	AssetManager();
	~AssetManager();

	static std::unique_ptr<AssetManager>	CreateInstance();

public:
	template<typename TClass, typename TRet, typename ...Args>
	void AttachMeshCreatedEvent(TRet(TClass::* func) (Args...), TClass* obj)
	{
		m_MeshManager.meshData.AttachMeshCreatedEvent(func, obj);
	};

	const SRVHeapData&		GetSRVHeapData() const;
	const TextureManager&	GetTextureManager() const;
	const MeshManager&		GetMeshManager() const;
	MaterialManager&		GetMaterialManager();

private:
	SRVHeapData				m_SrvHeapData;
	TextureManager			m_TextureManager;
	MaterialManager			m_MaterialManager;
	MeshManager				m_MeshManager;	
};