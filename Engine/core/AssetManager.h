#pragma once
#include "Mesh.h"
#include <Texture.h>
#include <Material.h>
#include <entt/entt.hpp>

using SRVHeapID = std::uint32_t;

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

using GlobalMeshInfo = std::vector<MeshInfo>;
using GlobalMaterialInfo = std::vector<MaterialInfo>;

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
	struct TextureManager
	{
		friend class AssetManager;
		friend class DeferredRenderer;	

		using TextureID = UINT;
	private:
		
		struct TextureTuple
		{
			Texture texture;
			SRVHeapID heapID{UINT_MAX};
		};

		struct TextureData
		{
			std::map<std::wstring, TextureID> textureMap;
			std::vector<TextureTuple> textures;
		} textureData;

	} m_TextureManager;

	struct MeshManager
	{
		friend class AssetManager;
		friend class DeferredRenderer;

		using MeshID = UINT;
		using InstanceID = UINT;

	private:
		//Per component data
		struct InstanceData
		{
			std::vector<MeshID> meshIds;
			std::vector<MeshInfo> meshInfo; //flags and material can change per instance
		} instanceData; //instance id

		struct MeshTuple
		{
			Mesh mesh;
			MeshInfo meshInfo; // default mesh info
			MaterialID materialId; // default material id, (i.e Assimp loading can populate this)
		};

		struct MeshData
		{
			const UINT globalFlags = UINT_MAX;
			std::map<std::wstring, MeshID> meshMap;
			std::vector<MeshTuple> meshes;
		}meshData;

		
	} m_MeshManager;

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