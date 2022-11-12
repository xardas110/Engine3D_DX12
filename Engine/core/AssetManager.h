#pragma once
#include "Mesh.h"
#include <Texture.h>
#include <Material.h>

//Every Textures and Meshes will be in the same heap
//Due to global shader access for inline DXR
struct SRVHeapData
{
	friend class DeferredRenderer;
private:
	std::uint32_t lastIndex = 0;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
	const int increment = 0;
	const int heapSize = 1024;

public:
	SRVHeapData();

	//Call this only once per resource!
	D3D12_CPU_DESCRIPTOR_HANDLE IncrementHandle(UINT& outCurrentHandleIndex)
	{
		outCurrentHandleIndex = lastIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += lastIndex++ * increment;
	}
};

class AssetManager
{
	friend class Application;
	friend class DeferredRenderer;
	friend class std::default_delete<AssetManager>;

	using TextureMap = std::map<std::wstring, TextureInstance>;
	using MeshMap = std::map<std::wstring, MeshInstance>;
	using MaterialMap = std::map<std::wstring, MaterialInstance>;
public:
	bool LoadStaticMesh(const std::string& path, StaticMesh& outStaticMesh);
	bool LoadTexture(const std::wstring& path, TextureInstance& outTextureInstance);
	bool GetMeshInstance(const std::wstring& name, MeshInstance& outMeshInstance);

	bool CreateMaterialInstance(const std::wstring& name, const MaterialInfo& textureIDs);
	bool GetMaterialInstance(const std::wstring& name, MaterialInstance& outMaterialInstance);
private:
	AssetManager();
	~AssetManager();

	//Gets called once in the constructor
	void CreateCube(const std::wstring& name = L"AssetManagerDefaultCube");

	static std::unique_ptr<AssetManager> CreateInstance();

	//Max number of descriptors in the heap is set to 1024
	//Make sure to increase it if more than 512 textures are used
	//The value is hardcoded during testing phase
	struct TextureData
	{
		friend class AssetManager;
		friend class DeferredRenderer;	
	private:
		TextureMap textureMap;
		Texture textures[512];
		std::uint32_t lastIndex = 0;
	} m_TextureData;

	struct MeshData
	{
		friend class AssetManager;
		friend class DeferredRenderer;
	private:
		MeshMap meshMap;
		Mesh meshes[512];
		std::uint32_t lastIndex = 0;
	} m_MeshData;

	struct MaterialData
	{
		friend class AssetManager;
		friend class DeferredRenderer;
	private:
		MaterialMap materialMap;
		Material materials[256];
		std::uint32_t lastIndex = 0;
	} m_MaterialData;

	SRVHeapData m_SrvHeapData;
};