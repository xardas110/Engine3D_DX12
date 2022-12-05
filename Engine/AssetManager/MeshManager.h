#pragma once
#include <Mesh.h>
#include <eventcpp/event.hpp>
#include <TypesCompat.h>

class SRVHeapData;

struct MeshManager
{
	friend class AssetManager;
	friend class MeshInstance;
	friend class Raytracing;
	friend class DeferredRenderer;
	friend class Editor;
	friend class StaticMesh;

	bool CreateMeshInstance(const std::wstring& path, MeshInstance& outMeshInstanceID);

	void CreateCube(const std::wstring& cubeName = L"DefaultCube");
	void CreateSphere(const std::wstring& sphereName = L"DefaultSphere");

	void CreateCone(const std::wstring& name = L"DefaultCone");

	void CreateTorus(const std::wstring& name = L"DefaultTorus");

	void LoadStaticMesh(const std::string& path, StaticMesh& outStaticMesh, MeshImport::Flags flags = MeshImport::None);
private:
	MeshManager(const SRVHeapData& srvHeapData);

	//Per component data
	struct InstanceData
	{
		friend class MeshManager;
		friend class MeshInstance;
		friend class Raytracing;
		friend class DeferredRenderer;
		friend class Editor;
		friend class StaticMesh;
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
		friend class Raytracing;
		friend class DeferredRenderer;
		friend class Editor;
		friend class MeshInstance;
		friend class StaticMesh;

		//Subscripe to this event to know about event creation
		event::event<void(const Mesh&)> meshCreationEvent;
	private:

		//Warning: Using this function will increment ref counter
		MeshID GetMeshID(const std::wstring& name);
		const std::wstring& GetName(MeshID id);


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