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
	auto cq = Application::Get().GetCommandQueue();
	auto cl = cq->GetCommandList();

	m_Primitives[Primitives::Cube] = *Mesh::CreateCube(*cl);

	auto fenceVal = cq->ExecuteCommandList(cl);
	cq->WaitForFenceValue(fenceVal);
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
