#pragma once
#include <Mesh.h>
#include <eventcpp/event.hpp>

class SRVHeapData;

struct MeshManager
{
	friend class AssetManager;
	friend class MeshInstance;
	friend class Raytracing;
	friend class DeferredRenderer;

	bool CreateMeshInstance(const std::wstring& path, MeshInstance& outMeshInstanceID);

	void CreateCube(const std::wstring& cubeName = L"DefaultCube");
	void CreateSphere(const std::wstring& sphereName = L"DefaultSphere");

private:
	MeshManager(const SRVHeapData& srvHeapData);

	//Per component data
	struct InstanceData
	{
		friend class MeshManager;
		friend class MeshInstance;
		friend class DeferredRenderer;
	private:

		MeshInstanceID CreateInstance(MeshID meshID, const MeshInfo& meshInfo);

		std::vector<MeshID> meshIds;
		std::vector<MeshInfo> meshInfo; //flags and material can change per instance
	} instanceData; //instance id

	struct MeshTuple
	{
		Mesh mesh;
		MeshInfo meshInfo; // default mesh info
	};

	struct MeshData
	{
		friend class MeshManager;
		friend class DeferredRenderer;

		//Subscripe to this event to know about event creation
		event::event<void(const Mesh&)> meshCreationEvent;
	private:

		//Warning: Using this function will increment ref counter
		MeshID GetMeshID(const std::wstring& name);

		void IncrementRef(const MeshID meshID);
		void DecrementRef(const MeshID meshID);

		void AddMesh(const std::wstring& name, MeshTuple& tuple);

		void CreateMesh(const std::wstring& name, MeshTuple& tuple, const SRVHeapData& heap);

		std::map<std::wstring, MeshID> map;
		std::vector<MeshTuple> meshes;
		std::vector<UINT> refCounter;
	} meshData;

	const SRVHeapData& m_SrvHeapData;
};