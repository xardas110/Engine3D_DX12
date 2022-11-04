#include <EnginePCH.h>
#include <AssetManager.h>
#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>

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
