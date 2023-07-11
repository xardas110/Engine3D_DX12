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

	bool					LoadTexture(const std::wstring& path, TextureInstance& outTextureInstance);
	void					LoadStaticMesh(	const std::string& path, 
											StaticMeshInstance& outStaticMesh, 
											MeshImport::Flags flags);

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