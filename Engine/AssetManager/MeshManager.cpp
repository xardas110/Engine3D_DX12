#include <EnginePCH.h>
#include <MeshManager.h>
#include <AssetManager.h>
#include <Application.h>
#include <CommandList.h>
#include <CommandQueue.h>

MeshManager::MeshManager(const SRVHeapData& srvHeapData)
	:m_SrvHeapData(srvHeapData)
{
	std::cout << "MeshManager running" << std::endl;
	CreateCube();
}

void MeshManager::CreateCube(const std::wstring& cubeName)
{
	auto commandQueue = Application::Get().GetCommandQueue();
	auto commandList = commandQueue->GetCommandList();

	MeshTuple tuple;
	tuple.mesh;

	//commandQueue->WaitForFenceValue(commandQueue->ExecuteCommandList(commandList));

	meshData.CreateMesh(cubeName, tuple, m_SrvHeapData);
}

bool MeshManager::CreateMeshInstance(const std::wstring& path, MeshInstance& outMeshInstanceID)
{
	assert(meshData.map.find(path) != meshData.map.end());

	if (meshData.map.find(path) != meshData.map.end())
	{
		const auto meshID = meshData.GetMeshID(path);
		const auto& meshInfo = meshData.meshes[meshID].meshInfo;
		
		outMeshInstanceID.id = instanceData.CreateInstance(meshID, meshInfo);

		return true;
	}

	return false;
}

MeshID MeshManager::MeshData::GetMeshID(const std::wstring& name)
{
	assert(map.find(name) != map.end() && "AssetManager::MeshManager::MeshData::GetMeshID name or path not found!");
	const auto id = map[name];
	IncrementRef(id);
	return id;
}

void MeshManager::MeshData::IncrementRef(const MeshID meshID)
{
	refCounter[meshID]++;
}

void MeshManager::MeshData::DecrementRef(const MeshID meshID)
{
	assert(refCounter[meshID] > 0U && "AssetManager::MeshManager::MeshData::DeReferenceMesh unable to de-refernce a mesh with no references");
	refCounter[meshID]--;
}

void MeshManager::MeshData::AddMesh(const std::wstring& name, MeshTuple& tuple)
{
	const auto currentIndex = meshes.size();
	map[name] = currentIndex;
	meshes.push_back(std::move(tuple));
	refCounter.emplace_back(UINT(1U));
}

void MeshManager::MeshData::CreateMesh(const std::wstring& name, MeshTuple& tuple, const SRVHeapData& heap)
{
	auto device = Application::Get().GetDevice();
	auto& mesh = tuple.mesh;
	auto& meshInfo = tuple.meshInfo;

	{
		auto cpuHandle = heap.IncrementHandle(meshInfo.vertexOffset);
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
		auto cpuHandle = heap.IncrementHandle(meshInfo.indexOffset);
		D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
		D3D12_BUFFER_SRV bufferSRV;
		bufferSRV.FirstElement = 0;
		bufferSRV.NumElements = mesh.m_IndexBuffer.GetNumIndicies();
		bufferSRV.StructureByteStride = sizeof(Index);
		bufferSRV.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		desc.Buffer = bufferSRV;
		desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		device->CreateShaderResourceView(mesh.m_IndexBuffer.GetD3D12Resource().Get(), &desc, cpuHandle);
	}

	AddMesh(name, tuple);
}

MeshInstanceID MeshManager::InstanceData::CreateInstance(MeshID meshID, const MeshInfo& meshInfo)
{
	const auto currentIndex = meshIds.size();
	meshIds.emplace_back(meshID);
	this->meshInfo.emplace_back(meshInfo);
	return (MeshInstanceID)currentIndex;
}
