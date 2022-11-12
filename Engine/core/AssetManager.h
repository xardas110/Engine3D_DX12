#pragma once
#include "Mesh.h"
#include <Texture.h>

//Every Textures and Meshes will be in the same heap
//Due to global shader access for inline DXR
struct SRVHeapData
{
private:
	std::uint32_t lastIndex = 0;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
	const int increment = 0;
	const int heapSize = 1024;
public:
	SRVHeapData();

	D3D12_CPU_DESCRIPTOR_HANDLE IncrementHandle(UINT& outCurrentHandleIndex)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += lastIndex++ * increment;
	}
};

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

	//Max number of descriptors in the heap is set to 1024
	//Make sure to increase it if more than 1024 textures are used
	//The value is hardcoded during testing phase
	struct TextureData
	{
		friend class AssetManager;
		friend class DeferredRenderer;
	
	private:
		TextureMap textureMap;
		Texture textures[512];
		UINT lastIndex = 0;
	} m_TextureData;

	struct MeshData
	{
	private:
		Mesh meshes[512];
		std::uint32_t lastIndex = 0;
	} m_MeshData;
};