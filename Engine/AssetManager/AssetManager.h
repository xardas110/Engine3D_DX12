#pragma once
#include "Mesh.h"
#include <Texture.h>
#include <Material.h>
#include <entt/entt.hpp>
#include <MeshManager.h>
#include <MaterialManager.h>
#include <TextureManager.h>

using RefCounter = UINT;

//Every Textures and Meshes will be in the same heap
//Due to global shader access for inline DXR
struct SRVHeapData
{
	friend class TextureManager;
	friend class MeshManager;
	friend class DeferredRenderer;
private:	
	mutable size_t lastIndex = 0;
	const size_t increment = 0;
	const int heapSize = 1024;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;	

public:
	SRVHeapData();

	//Call this only once per resource!
	D3D12_CPU_DESCRIPTOR_HANDLE IncrementHandle(UINT& outCurrentHandleIndex) const
	{
		outCurrentHandleIndex = lastIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += lastIndex++ * increment;
		return handle;
	}
};

class AssetManager
{
	friend class Application;
	friend class TextureInstance;
	friend class MaterialInstance;
	friend class MeshInstance;
	friend class DeferredRenderer;
	friend class std::default_delete<AssetManager>;

public:


private:
	AssetManager();
	~AssetManager();

	static std::unique_ptr<AssetManager> CreateInstance();

	MeshManager m_MeshManager;
	MaterialManager m_MaterialManager;
	TextureManager m_TextureManager;
	
	SRVHeapData m_SrvHeapData;
};