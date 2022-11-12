#include <EnginePCH.h>
#include <AssetManager.h>
#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <AssimpLoader.h>

bool IsMaterialValid(UINT id)
{
	return id != UINT_MAX;
}

SRVHeapData::SRVHeapData()
	: increment(Application::Get().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
{
	auto device = Application::Get().GetDevice();
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = 1024;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
}

std::wstring StringToWString(const std::string& s)
{
	return std::wstring(s.begin(), s.end());
}

AssetManager::AssetManager()
{
	CreateCube();
}

void AssetManager::CreateCube(const std::wstring& name)
{
	if (m_MeshData.meshMap.find(name) != m_MeshData.meshMap.end())
	{
		return;
	}

	auto device = Application::Get().GetDevice();
	auto cq = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto cl = cq->GetCommandList();

	m_MeshData.meshes[m_MeshData.lastIndex] = *Mesh::CreateCube(*cl);	
	auto& internalMesh = m_MeshData.meshes[m_MeshData.lastIndex];

	auto vertexOffset = 0U; auto indexOffset = 0U;
	{
		auto cpuHandle = m_SrvHeapData.IncrementHandle(vertexOffset);
		D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
		D3D12_BUFFER_SRV bufferSRV;
		bufferSRV.FirstElement = 0;
		bufferSRV.NumElements = internalMesh.m_VertexBuffer.GetNumVertices();
		bufferSRV.StructureByteStride = sizeof(VertexPositionNormalTexture);
		bufferSRV.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		desc.Buffer = bufferSRV;
		desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		device->CreateShaderResourceView(internalMesh.m_VertexBuffer.GetD3D12Resource().Get(), &desc, cpuHandle);
	}
	{
		auto cpuHandle = m_SrvHeapData.IncrementHandle(indexOffset);
		D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
		D3D12_BUFFER_SRV bufferSRV;
		bufferSRV.FirstElement = 0;
		bufferSRV.NumElements = internalMesh.m_IndexBuffer.GetNumIndicies();
		bufferSRV.StructureByteStride = sizeof(Index);
		bufferSRV.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		desc.Buffer = bufferSRV;
		desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		device->CreateShaderResourceView(internalMesh.m_IndexBuffer.GetD3D12Resource().Get(), &desc, cpuHandle);
	}

	MeshInstance meshInstance;
	meshInstance.meshID = m_MeshData.lastIndex;
	meshInstance.meshInfo.vertexOffset = vertexOffset;
	meshInstance.meshInfo.indexOffset = indexOffset;

	m_MeshData.meshMap[name] = meshInstance;
	m_MeshData.lastIndex++;
}

AssetManager::~AssetManager()
{
	//m_Primitives will be deleted once its unreferenced.
}

std::unique_ptr<AssetManager> AssetManager::CreateInstance()
{
	return std::unique_ptr<AssetManager>(new AssetManager);
}

bool AssetManager::LoadTexture(const std::wstring& path, TextureInstance& outTextureInstance)
{
	if (m_TextureData.textureMap.find(path) != m_TextureData.textureMap.end())
	{
		outTextureInstance = m_TextureData.textureMap[path];
		return true;
	}

	auto device = Application::Get().GetDevice();
	auto cq = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto cl = cq->GetCommandList();

	outTextureInstance.textureID = m_TextureData.lastIndex;
	auto& texture = m_TextureData.textures[m_TextureData.lastIndex];
	cl->LoadTextureFromFile(texture, path);

	auto cpuHandle = m_SrvHeapData.IncrementHandle(outTextureInstance.srvHeapID);
	device->CreateShaderResourceView(texture.GetD3D12Resource().Get(), nullptr, cpuHandle);

	auto fenceVal = cq->ExecuteCommandList(cl);
	cq->WaitForFenceValue(fenceVal);

	m_TextureData.textureMap[path] = outTextureInstance;
	m_TextureData.lastIndex++;

	return true;
}

bool AssetManager::GetMeshInstance(const std::wstring& name, MeshInstance& meshInstance)
{
	if (m_MeshData.meshMap.find(name) == m_MeshData.meshMap.end())
		return false;

	meshInstance = m_MeshData.meshMap[name];
	return true;
}

bool AssetManager::CreateMaterialInstance(const std::wstring& name, const MaterialInfo& textureIDs)
{
	assert(m_MaterialData.materialMap.find(name) == m_MaterialData.materialMap.end() && "Material instance exists!");
	if (m_MaterialData.materialMap.find(name) != m_MaterialData.materialMap.end()) return false;

	Material material;

	if (IsMaterialValid(textureIDs.albedo))
	{
		material.textureIDs.albedo = textureIDs.albedo;
	}
	if (IsMaterialValid(textureIDs.normal))
	{
		material.textureIDs.normal = textureIDs.normal;
	}

	m_MaterialData.materials[m_MaterialData.lastIndex] = material;
	m_MaterialData.materialMap[name].materialID = m_MaterialData.lastIndex;
	m_MaterialData.lastIndex++;

	return true;
}

bool AssetManager::GetMaterialInstance(const std::wstring& name, MaterialInstance& outMaterialInstance)
{
	assert(m_MaterialData.materialMap.find(name) != m_MaterialData.materialMap.end() && "Unable to find material instance");

	if (m_MaterialData.materialMap.find(name) == m_MaterialData.materialMap.end()) return false;

	outMaterialInstance = m_MaterialData.materialMap[name];

	return true;
}

bool AssetManager::LoadStaticMesh(const std::string& path, StaticMesh& outStaticMesh)
{
	/*
	if (staticMeshMap.find(path) != staticMeshMap.end())
	{
		outStaticMesh = staticMeshMap[path];
		return true;
	}

	auto cq = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto cl = cq->GetCommandList();

	AssimpLoader loader(path);

	auto& sm = loader.GetAssimpStaticMesh();

	auto startIndex = lastIndex;

	for (int i = 0; i < sm.meshes.size(); i++)
	{
		auto& assimpMesh = sm.meshes[i];
		auto& mesh = m_Meshes[i + startIndex];
		mesh = *Mesh::CreateMesh(*cl, assimpMesh.vertices, assimpMesh.indices, false);

		for (size_t i = 0; i < AssimpMaterialType::Size; i++)
		{
			if (assimpMesh.material.textures[i].IsValid())
			{
				cl->LoadTextureFromFile(mesh.m_Material.textures[i], StringToWString(assimpMesh.material.textures[i].path));
			}
		}
	}

	outStaticMesh.startOffset = startIndex;
	outStaticMesh.count = sm.meshes.size();

	lastIndex = startIndex + sm.meshes.size();

	staticMeshMap[path] = outStaticMesh;

	auto fenceVal = cq->ExecuteCommandList(cl);
	cq->WaitForFenceValue(fenceVal);
	*/
	return true;
}