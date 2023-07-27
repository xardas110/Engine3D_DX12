#pragma once

#include <Mesh.h>
#include <eventcpp/event.hpp>
#include <TypesCompat.h>
#include <AssetManagerDefines.h>
#include <type_traits>

#define MESH_GPU_HEAP_HANDLE_INVALID 0xffffffff

class SRVHeapData;
class MeshManager;
struct MaterialInstance;

class MeshManagerAccess
{
	friend class AssetManager;
private:
	static std::unique_ptr<MeshManager> CreateMeshManager(const SRVHeapData& srvHeapData);
};

class MeshManager
{
	friend struct MeshInstance;
	friend class MeshManagerAccess;
	friend class MeshManagerTest;
private:

	MeshManager(const SRVHeapData& srvHeapData);

	MeshManager(const MeshManager&) = delete;
	MeshManager& operator=(const MeshManager&) = delete;

	MeshManager(MeshManager&&) = delete;
	MeshManager& operator=(MeshManager&&) = delete;

	~MeshManager() = default;

public:

	std::optional<MeshInstance> CreateMesh(
		const std::wstring& name,
		std::shared_ptr<CommandList> commandList, 
		std::shared_ptr<CommandList> rtCommandList, 
		VertexCollection& vertices, IndexCollection32& indices, 
		bool rhcoords, bool calcTangent);

	bool IsMeshValid(const std::wstring& name) const;
	bool IsMeshValid(const MeshInstance meshInstance) const;
	template<bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
	bool IsMeshValid(std::conditional_t<ThreadSafe, const Mesh, const Mesh*>) const;

	void SetFlags(const MeshInstance meshInstance, const UINT meshFlags);
	void SetFlags(const std::wstring& meshName, const UINT meshFlags);

	void AddFlags(const MeshInstance meshInstance, const UINT meshFlags);
	void AddFlags(const std::wstring& meshName, const UINT meshFlags);

	std::optional<MeshInstance> GetMeshInstance(const std::wstring& name) const;
	std::optional<std::wstring> GetMeshName(const MeshInstance meshInstance) const;
	std::optional<MeshRefCount> GetRefCount(const MeshInstance meshInstance) const;
	std::optional<UINT>			GetMeshFlags(const MeshInstance meshInstance) const;

	template<bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
	auto GetMesh(const MeshInstance meshInstance) const
		->std::conditional_t<ThreadSafe, const Mesh*, const Mesh*>;

	template<bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
	auto GetMesh(const std::wstring& name) const
		->std::conditional_t<ThreadSafe, const Mesh*, const Mesh*>;

	template<bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
	auto GetMeshGPUHandle(const MeshInstance meshInstance) const
		->std::conditional_t<ThreadSafe, const struct MeshInfo, const MeshInfo&>;

	template<bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
	auto GetMeshData() const
		-> std::conditional_t<ThreadSafe, const std::vector<Mesh>, const std::vector<Mesh>&>;

	template<bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
	auto GetMeshGPUHandlesData() const
		->std::conditional_t<ThreadSafe, const std::vector<struct MeshInfo>, const std::vector<struct MeshInfo>&>;

private:
	
	bool IsMeshValid(const MeshID meshID) const;
	void IncreaseRefCount(const MeshID meshID);
	void DecreaseRefCount(const MeshID meshID);
	void ReleaseMesh(const MeshID meshID);

	struct MeshRegistry
	{
		MeshRegistry() = default;

		MeshRegistry(const MeshRegistry&) = delete;
		MeshRegistry& operator=(const MeshRegistry&) = delete;

		MeshRegistry(MeshRegistry&&) = delete;
		MeshRegistry& operator=(MeshRegistry&&) = delete;

		~MeshRegistry() = default;

		std::vector<Mesh> meshes;		
		std::vector<MeshInfo> gpuHeapHandles;
		std::unordered_map<std::wstring, MeshInstance> map;
		std::array<std::atomic<MeshRefCount>, MESH_MANAGER_MAX_MESHES> refCounts;
	
		CREATE_MUTEX(meshes);
		CREATE_MUTEX(flags);
		CREATE_MUTEX(gpuHeapHandles);
		CREATE_MUTEX(map);

	} meshRegistry;

	std::vector<MeshID> releasedMeshIDs;
	CREATE_MUTEX(releasedMeshIDs);

	const SRVHeapData& srvHeapData;
};

template<bool ThreadSafe>
bool MeshManager::IsMeshValid(std::conditional_t<ThreadSafe, const Mesh, const Mesh*> mesh) const
{
	if constexpr (!ThreadSafe)
	{
		if (!mesh)
			return false;

		return mesh->m_VertexBuffer.IsValid();
	}

	return mesh.m_VertexBuffer.IsValid();
}


template<bool ThreadSafe>
inline auto MeshManager::GetMesh(const MeshInstance meshInstance) const
->std::conditional_t<ThreadSafe, const Mesh*, const Mesh*>
{
	if (IsMeshValid(meshInstance.id))
		return nullptr;

	SHARED_LOCK(MeshRegistryMeshes, meshRegistry.meshesMutex);
	return &meshRegistry.meshes[meshInstance.id];
}

template<bool ThreadSafe>
inline auto MeshManager::GetMesh(const std::wstring& name) const
->std::conditional_t<ThreadSafe, const Mesh*, const Mesh*>
{
	if (!HasMesh(name))
		return nullptr;

	SHARED_LOCK(MeshRegistryMeshMap, meshRegistry.mapMutex);
	auto meshInstance = meshRegistry.map[name];
	SHARED_UNLOCK(MeshRegistryMeshMap);

	if (!IsMeshValid(meshInstance))
		return nullptr;

	SHARED_LOCK(MeshRegistryMeshes, meshRegistry.meshesMutex);
	return &meshRegistry.meshes[meshInstance.id];
}

template<bool ThreadSafe>
auto MeshManager::GetMeshGPUHandle(const MeshInstance meshInstance) const
->std::conditional_t<ThreadSafe, const struct MeshInfo, const MeshInfo&>
{
	if (!meshInstance.IsValid())
		return MeshInfo();

	SHARED_LOCK(MeshRegistryGPUHandle, meshRegistry.gpuHeapHandles);
	return meshRegistry.gpuHeapHandles[meshInstance.id];
}

template<bool ThreadSafe>
inline auto MeshManager::GetMeshData() const
-> std::conditional_t<ThreadSafe, const std::vector<Mesh>, const std::vector<Mesh>&>
{
	SHARED_LOCK(MeshRegistryMeshes, meshRegistry.meshesMutex);
	return meshRegistry.meshes;
}

template<bool ThreadSafe>
inline auto MeshManager::GetMeshGPUHandlesData() const
-> std::conditional_t<ThreadSafe, const std::vector<MeshInfo>, const std::vector<MeshInfo>&>
{
	SHARED_LOCK(MeshRegistryMeshGPUHeaphandles, meshRegistry.gpuHeapHandlesMutex);
	return meshRegistry.gpuHeapHandles;
}

