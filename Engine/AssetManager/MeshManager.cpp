#include <EnginePCH.h>
#include <MeshManager.h>
#include <AssetManager.h>
#include <Application.h>
#include <CommandList.h>
#include <CommandQueue.h>
#include <AssimpLoader.h>
#include <Material.h>

const std::wstring g_NoName = L"No Name";

void ApplyMaterialInstanceImportFlags(MaterialInstance& matInstance, MeshImport::Flags flags, MaterialInfo matInfo)
{
	matInstance.SetFlags(INSTANCE_OPAQUE);

	if (matInfo.opacity != UINT_MAX)
	{
		matInstance.SetFlags(INSTANCE_TRANSLUCENT);

		if (MeshImport::ForceAlphaBlend & flags)
			matInstance.AddFlag(INSTANCE_ALPHA_BLEND);
		else if (MeshImport::ForceAlphaCutoff & flags)
			matInstance.AddFlag(INSTANCE_ALPHA_CUTOFF);
	}

	if (flags & MeshImport::AO_Rough_Metal_As_Spec_Tex)
	{
		matInstance.AddFlag(MATERIAL_FLAG_AO_ROUGH_METAL_AS_SPECULAR);
	}

	if (flags & MeshImport::BaseColorAlpha)
	{
		matInstance.AddFlag(MATERIAL_FLAG_BASECOLOR_ALPHA);
	}

	if (flags & MeshImport::InvertMaterialNormals)
	{
		matInstance.AddFlag(MATERIAL_FLAG_INVERT_NORMALS);
	}
}

MeshManager::MeshManager(const SRVHeapData& srvHeapData)
	:m_SrvHeapData(srvHeapData)
{}

void MeshManager::LoadStaticMesh(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, const std::string& path, StaticMeshInstance& outStaticMesh, MeshImport::Flags flags)
{
	AssimpLoader loader(path, flags);

	const auto& sm = loader.GetAssimpStaticMesh();

	std::cout << "Num meshes in static mesh: " << sm.meshes.size() << std::endl;

	outStaticMesh.startOffset = instanceData.meshInfo.size();

	int numMeshes = 0;
	for (auto mesh : sm.meshes)
	{
		std::wstring currentName = std::wstring(path.begin(), path.end()) + L"/" + std::to_wstring(numMeshes++) + L"/" + std::wstring(mesh.name.begin(), mesh.name.end());

		auto internalMesh = Mesh::CreateMesh(commandList, rtCommandList, mesh.vertices, mesh.indices, (MeshImport::RHCoords & flags), MeshImport::CustomTangent & flags);
		meshData.CreateMesh(currentName, std::move(internalMesh), m_SrvHeapData);

		MeshInstance meshInstance(currentName);
		MaterialInfo matInfo = MaterialInfoHelper::PopulateMaterialInfo(mesh, flags);
		MaterialInstance matInstance(currentName, matInfo);
		ApplyMaterialInstanceImportFlags(matInstance, flags, matInfo);

		if (mesh.materialData.bHasMaterial)
		{
			Material materialData = MaterialHelper::CreateMaterial(mesh.materialData);

			if (mesh.materialData.opacity < 1.f)
			{
				matInstance.SetFlags(INSTANCE_TRANSLUCENT);

				if (MeshImport::ForceAlphaBlend & flags)
					matInstance.AddFlag(INSTANCE_ALPHA_BLEND);
				else if (MeshImport::ForceAlphaCutoff & flags)
					matInstance.AddFlag(INSTANCE_ALPHA_CUTOFF);
			}

			const auto materialID = MaterialInstance::CreateMaterial(currentName + L"/" + std::wstring(mesh.materialData.name.begin(), mesh.materialData.name.end()), materialData);
			matInstance.SetMaterial(materialID);
		}

		meshInstance.SetMaterialInstance(matInstance);
	}

	outStaticMesh.endOffset = outStaticMesh.startOffset + numMeshes;
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
	return id;
}

const std::wstring& MeshManager::MeshData::GetName(MeshID id)
{
	for (const auto& [name, mId] : map)
	{
		if (id == mId) return name;
	}

	return g_NoName;
}

void MeshManager::MeshData::AddMesh(const std::wstring& name, MeshTuple& tuple)
{
	const auto currentIndex = meshes.size();
	map[name] = currentIndex;
	meshes.emplace_back(std::move(tuple));
}

MeshManager::MeshTuple MeshManager::MeshData::CreateMesh(const std::wstring& name, Mesh&& mesh, const SRVHeapData& heap)
{
	MeshInfo meshInfo;

	auto device = Application::Get().GetDevice();

	{
		const auto cpuHandle = heap.IncrementHandle(meshInfo.vertexOffset);
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
		const auto cpuHandle = heap.IncrementHandle(meshInfo.indexOffset);

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

	MeshTuple meshTuple(std::move(mesh), meshInfo);

	meshCreationEvent(meshTuple.mesh);

	AddMesh(name, meshTuple);

	return meshTuple;
}


MeshInstanceID MeshManager::InstanceData::CreateInstance(MeshID meshID, const MeshInfo& meshInfo)
{
	const auto currentIndex = meshIds.size();
	meshIds.emplace_back(meshID);
	this->meshInfo.emplace_back(meshInfo);
	return (MeshInstanceID)currentIndex;
}
