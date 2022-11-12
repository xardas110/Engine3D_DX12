#include <EnginePCH.h>
#include <AssetManager.h>
#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <AssimpLoader.h>

std::wstring StringToWString(const std::string& s)
{
	return std::wstring(s.begin(), s.end());
}

AssetManager::AssetManager()
{
	auto device = Application::Get().GetDevice();
	auto cq = Application::Get().GetCommandQueue();
	auto cl = cq->GetCommandList();

	m_Primitives[Primitives::Cube] = *Mesh::CreateCube(*cl);
	auto fenceVal = cq->ExecuteCommandList(cl);
	cq->WaitForFenceValue(fenceVal);

	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = 1024;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_TextureData.heap));
	m_TextureData.increment = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

AssetManager::~AssetManager()
{
	//m_Primitives will be deleted once its unreferenced.
}

std::unique_ptr<AssetManager> AssetManager::CreateInstance()
{
	return std::unique_ptr<AssetManager>(new AssetManager);
}

bool AssetManager::LoadStaticMesh(const std::string& path, StaticMesh& outStaticMesh)
{
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

	return true;
}


bool AssetManager::LoadTexture(const std::wstring& path, TextureID& outTextureID)
{
	if (m_TextureData.textureMap.find(path) != m_TextureData.textureMap.end())
	{
		return m_TextureData.textureMap[path];
	}

	auto device = Application::Get().GetDevice();
	auto cq = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto cl = cq->GetCommandList();

	outTextureID = lastIndex;
	m_TextureData.textureMap[path] = lastIndex;
	auto& texture = m_TextureData.m_Textures[lastIndex];
	cl->LoadTextureFromFile(texture, path);

	auto cpuHandle = m_TextureData.heap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += (m_TextureData.increment * lastIndex);

	device->CreateShaderResourceView(texture.GetD3D12Resource().Get(), nullptr, cpuHandle);

	auto fenceVal = cq->ExecuteCommandList(cl);
	cq->WaitForFenceValue(fenceVal);

	lastIndex++;

	return true;
}