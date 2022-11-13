#pragma once
#include <Mesh.h>

class SRVHeapData;

struct MeshManager
{
	friend class AssetManager;
	friend class DeferredRenderer;
private:

	MeshManager(const SRVHeapData& srvHeapData);

	//Per component data
	struct InstanceData
	{
		friend class AssetManager;
		friend class DeferredRenderer;
	private:

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
		friend class AssetManager;
		friend class DeferredRenderer;
	private:
		//Warning: Using this function will increment ref counter
		MeshID GetMeshID(const std::wstring& name);

		void IncrementRef(const MeshID meshID);
		void DecrementRef(const MeshID meshID);

		void AddMesh(const std::wstring& name, MeshTuple&& tuple);

		void CreateMesh(const std::wstring& name, MeshTuple&& tuple, SRVHeapData& heap);

		std::map<std::wstring, MeshID> map;
		std::vector<MeshTuple> meshes;
		std::vector<UINT> refCounter;
	} meshData;

	const SRVHeapData& m_SrvHeapData;
};