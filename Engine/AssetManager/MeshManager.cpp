#include <EnginePCH.h>
#include <MeshManager.h>
#include <AssetManager.h>
#include <Application.h>
#include <CommandList.h>
#include <CommandQueue.h>
#include <AssimpLoader.h>
#include <Material.h>
#include <Logger.h>
#include <TypesCompat.h>

std::unique_ptr<MeshManager> MeshManagerAccess::CreateMeshManager(const SRVHeapData& srvHeapData)
{
	return std::unique_ptr<MeshManager>(new MeshManager(srvHeapData));
}

MeshManager::MeshManager(const SRVHeapData& srvHeapData)
	:srvHeapData(srvHeapData)
{}

std::optional<MeshInstance> MeshManager::CreateMesh(
	const std::wstring& name,
	std::shared_ptr<CommandList> commandList,
	std::shared_ptr<CommandList> rtCommandList,
	VertexCollection& vertices, IndexCollection32& indices,
	bool rhcoords, bool calcTangent)
{
	if (IsMeshValid(name))
	{
		LOG_INFO("Mesh exists with name: %s, returning an instance", std::string(name.begin(), name.end()).c_str());
		return GetMeshInstance(name);
	}

	MeshInfo gpuHeapHandles;
	auto device = Application::Get().GetDevice();
	auto mesh = Mesh::CreateMesh(*commandList, rtCommandList, vertices, indices, rhcoords, calcTangent);
	{
		const auto cpuHandle = srvHeapData.IncrementHandle(gpuHeapHandles.vertexOffset);
		D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
		D3D12_BUFFER_SRV bufferSRV;
		bufferSRV.FirstElement = 0;
		bufferSRV.NumElements = mesh.m_VertexBuffer.GetNumVertices();
		bufferSRV.StructureByteStride = sizeof(VertexPositionNormalTexture);
		bufferSRV.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		desc.Buffer = bufferSRV;
		desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		device->CreateShaderResourceView(mesh.m_VertexBuffer.GetD3D12Resource().Get(), &desc, cpuHandle);
	}
	{
		const auto cpuHandle = srvHeapData.IncrementHandle(gpuHeapHandles.indexOffset);

		D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
		D3D12_BUFFER_SRV bufferSRV;
		bufferSRV.FirstElement = 0;
		bufferSRV.NumElements = mesh.m_IndexBuffer.GetNumIndicies();
		bufferSRV.StructureByteStride = sizeof(UINT);
		bufferSRV.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		desc.Buffer = bufferSRV;
		desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		device->CreateShaderResourceView(mesh.m_IndexBuffer.GetD3D12Resource().Get(), &desc, cpuHandle);
	}
	
	auto currentIndex = 0;
	{
		UNIQUE_LOCK(releasedMesh, releasedMeshIDsMutex);
		if (!releasedMeshIDs.empty())
		{
			currentIndex = releasedMeshIDs.front();
			releasedMeshIDs.erase(releasedMeshIDs.begin());
		}
		else
		{
			currentIndex = meshRegistry.meshes.size();
		}
	}
	{		
		{
			UNIQUE_LOCK(meshes, meshRegistry.meshesMutex);
			meshRegistry.meshes.emplace_back(std::move(mesh));
		}	
		{
			UNIQUE_LOCK(map, meshRegistry.mapMutex);
			meshRegistry.map[name] = MeshInstance(currentIndex);
		}
		{
			UNIQUE_LOCK(gpuHeapHandles, meshRegistry.gpuHeapHandlesMutex);
			meshRegistry.gpuHeapHandles.emplace_back(gpuHeapHandles);
		}
		
		meshRegistry.refCounts[currentIndex].store(0u);
	}

	SHARED_LOCK(map, meshRegistry.mapMutex);
	meshInstanceCreatedEvent(meshRegistry.map[name]);
	return meshRegistry.map[name];
}

void MeshManager::SetFlags(const MeshInstance meshInstance, const UINT meshFlags)
{
	if (!meshInstance.IsValid())
		return;

	UNIQUE_LOCK(GpuHandles, meshRegistry.gpuHeapHandlesMutex);
	meshRegistry.gpuHeapHandles[meshInstance.id].flags = meshFlags;
}

void MeshManager::SetFlags(const std::wstring& meshName, const UINT meshFlags)
{
	const auto meshInstance = GetMeshInstance(meshName);

	if (!meshInstance.has_value())
		return;

	if (!meshInstance.value().IsValid())
		return;

	UNIQUE_LOCK(GpuHandles, meshRegistry.gpuHeapHandlesMutex);
	meshRegistry.gpuHeapHandles[meshInstance.value().id].flags = meshFlags;
}

void MeshManager::AddFlags(const MeshInstance meshInstance, const UINT meshFlags)
{
	if (!meshInstance.IsValid())
		return;

	UNIQUE_LOCK(GpuHandles, meshRegistry.gpuHeapHandlesMutex);
	meshRegistry.gpuHeapHandles[meshInstance.id].flags |= meshFlags;
}

void MeshManager::AddFlags(const std::wstring& meshName, const UINT meshFlags)
{
	const auto meshInstance = GetMeshInstance(meshName);

	if (!meshInstance.has_value())
		return;

	if (!meshInstance.value().IsValid())
		return;

	UNIQUE_LOCK(GpuHandles, meshRegistry.gpuHeapHandlesMutex);
	meshRegistry.gpuHeapHandles[meshInstance.value().id].flags |= meshFlags;
}

std::optional<MeshRefCount> MeshManager::GetRefCount(const MeshInstance meshInstance) const
{
	if (!IsMeshValid(meshInstance))
		return std::nullopt;

	return meshRegistry.refCounts[meshInstance.id];
}

std::optional<UINT> MeshManager::GetMeshFlags(const MeshInstance meshInstance) const
{
	if (!IsMeshValid(meshInstance))
		return std::nullopt;

	SHARED_LOCK(GpuHandles, meshRegistry.gpuHeapHandlesMutex);
	return meshRegistry.gpuHeapHandles[meshInstance.id].flags;
}

bool MeshManager::IsMeshValid(const MeshInstance meshInstance) const
{
	return IsMeshValid(meshInstance.id);
}

bool MeshManager::IsMeshValid(const std::wstring& name) const
{
	MeshInstance result;
	{
		SHARED_LOCK(map, meshRegistry.mapMutex);
		auto it = meshRegistry.map.find(name);
		if (it == meshRegistry.map.end())
			return false;

		result = it->second;
	}
	return IsMeshValid(result);
}

bool MeshManager::IsMeshValid(const MeshID meshID) const
{
	if (meshID == MESH_INVALID)
		return false;

	{
		SHARED_LOCK(meshes, meshRegistry.meshesMutex);
		if (meshID >= meshRegistry.meshes.size())
			return false;

		if (!meshRegistry.meshes[meshID].m_VertexBuffer.IsValid())
			return false;
	}

	return true;
}

void MeshManager::IncreaseRefCount(const MeshID meshID)
{
	if (IsMeshValid(meshID))
	{
		meshRegistry.refCounts[meshID].fetch_add(1);
	}
}

void MeshManager::DecreaseRefCount(const MeshID meshID)
{
	if (!IsMeshValid(meshID))
		return;
	
	if (meshRegistry.refCounts[meshID].fetch_sub(1) != 1)
		return;

	ReleaseMesh(meshID);
}

void MeshManager::ReleaseMesh(const MeshID meshID)
{	
	LOG_INFO("Releaseing mesh with id: %i", (int)meshID);	
	{
		UNIQUE_LOCK(releasedMeshIDs, releasedMeshIDsMutex);
		releasedMeshIDs.emplace_back(meshID);
	}
	{
		UNIQUE_LOCK(meshes, meshRegistry.meshesMutex);
		meshRegistry.meshes[meshID] = Mesh();
	}	
	{
		UNIQUE_LOCK(gpuHeapHandles, meshRegistry.gpuHeapHandlesMutex);
		meshRegistry.gpuHeapHandles[meshID] = MeshInfo();
	}	
	{
		UNIQUE_LOCK(map, meshRegistry.mapMutex);	
		for (auto it = meshRegistry.map.begin(); it != meshRegistry.map.end(); )
		{
			if (it->second.id == meshID)
			{
				it = meshRegistry.map.erase(it);
			}
			else
			{
				++it;
			}
		};
	}
	meshRegistry.refCounts[meshID].store(0U);

	meshInstanceDeletedEvent(meshID);
}

std::optional<MeshInstance> MeshManager::GetMeshInstance(const std::wstring& name) const
{
	if (!IsMeshValid(name))
		return std::nullopt;

	SHARED_LOCK(map, meshRegistry.mapMutex);
	auto it = meshRegistry.map.find(name);
	if (it != meshRegistry.map.end())
	{
		return it->second;
	}

	return std::nullopt;
}

std::optional<std::wstring> MeshManager::GetMeshName(const MeshInstance meshInstance) const
{
	if (!IsMeshValid(meshInstance))
		return std::nullopt;

	SHARED_LOCK(map, meshRegistry.mapMutex);
	for (auto [name, instance] : meshRegistry.map)
	{
		if (instance.id == meshInstance.id)
			return name;
	}
	
	return std::nullopt;
}
