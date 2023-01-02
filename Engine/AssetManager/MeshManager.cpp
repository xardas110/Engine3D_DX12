#include <EnginePCH.h>
#include <MeshManager.h>
#include <AssetManager.h>
#include <Application.h>
#include <CommandList.h>
#include <CommandQueue.h>
#include <AssimpLoader.h>
#include <Material.h>

const std::wstring g_NoName = L"No Name";

MeshManager::MeshManager(const SRVHeapData& srvHeapData)
	:m_SrvHeapData(srvHeapData)
{
}

void MeshManager::CreateCube(const std::wstring& cubeName)
{
	auto commandQueue = Application::Get().GetCommandQueue();
	auto commandList = commandQueue->GetCommandList();
	auto rtCommandList = commandQueue->GetCommandList();

	MeshTuple tuple;
	tuple.mesh = std::move(*Mesh::CreateCube(*commandList));
	tuple.mesh.InitializeBlas(*rtCommandList);

	auto fenceVal1 = commandQueue->ExecuteCommandList(commandList);
	auto fenceVal2 = commandQueue->ExecuteCommandList(rtCommandList);

	commandQueue->WaitForFenceValue(fenceVal1);
	commandQueue->WaitForFenceValue(fenceVal2);

	meshData.CreateMesh(cubeName, tuple, m_SrvHeapData);
}

void MeshManager::CreateSphere(const std::wstring& sphereName)
{
	auto commandQueue = Application::Get().GetCommandQueue();
	auto commandList = commandQueue->GetCommandList();
	auto rtCommandList = commandQueue->GetCommandList();

	MeshTuple tuple;
	tuple.mesh = std::move(*Mesh::CreateSphere(*commandList));
	tuple.mesh.InitializeBlas(*rtCommandList);

	auto fenceVal1 = commandQueue->ExecuteCommandList(commandList);
	auto fenceVal2 = commandQueue->ExecuteCommandList(rtCommandList);

	commandQueue->WaitForFenceValue(fenceVal1);
	commandQueue->WaitForFenceValue(fenceVal2);

	meshData.CreateMesh(sphereName, tuple, m_SrvHeapData);
}

void MeshManager::CreateCone(const std::wstring& name)
{
	auto commandQueue = Application::Get().GetCommandQueue();
	auto commandList = commandQueue->GetCommandList();
	auto rtCommandList = commandQueue->GetCommandList();

	MeshTuple tuple;
	tuple.mesh = std::move(*Mesh::CreateCone(*commandList));
	tuple.mesh.InitializeBlas(*rtCommandList);

	auto fenceVal1 = commandQueue->ExecuteCommandList(commandList);
	auto fenceVal2 = commandQueue->ExecuteCommandList(rtCommandList);

	commandQueue->WaitForFenceValue(fenceVal1);
	commandQueue->WaitForFenceValue(fenceVal2);

	meshData.CreateMesh(name, tuple, m_SrvHeapData);
}

void MeshManager::CreateTorus(const std::wstring& name)
{
	auto commandQueue = Application::Get().GetCommandQueue();
	auto commandList = commandQueue->GetCommandList();
	auto rtCommandList = commandQueue->GetCommandList();

	MeshTuple tuple;
	tuple.mesh = std::move(*Mesh::CreateTorus(*commandList));
	tuple.mesh.InitializeBlas(*rtCommandList);

	auto fenceVal1 = commandQueue->ExecuteCommandList(commandList);
	auto fenceVal2 = commandQueue->ExecuteCommandList(rtCommandList);

	commandQueue->WaitForFenceValue(fenceVal1);
	commandQueue->WaitForFenceValue(fenceVal2);

	meshData.CreateMesh(name, tuple, m_SrvHeapData);
}

void MeshManager::LoadStaticMesh(const std::string& path, StaticMesh& outStaticMesh, MeshImport::Flags flags)
{
	auto commandQueue = Application::Get().GetCommandQueue();
	auto commandList = commandQueue->GetCommandList();
	auto rtCommandList = commandQueue->GetCommandList();
	auto& textureManager = Application::Get().GetAssetManager()->m_TextureManager;

	AssimpLoader loader(path, flags);

	auto sm = loader.GetAssimpStaticMesh();
	outStaticMesh.startOffset = instanceData.meshInfo.size();

	std::cout << "Num meshes in static mesh: " << sm.meshes.size() << std::endl;

	int num = 0; 
	for (auto& mesh : sm.meshes)
	{		
		std::wstring currentName = std::wstring(path.begin(), path.end()) + L"/" + std::to_wstring(num++) + L"/" + std::wstring(mesh.name.begin(), mesh.name.end());

		MeshTuple tuple;
		tuple.mesh = std::move(*Mesh::CreateMesh(*commandList, mesh.vertices, mesh.indices, !(MeshImport::LHCoords & flags)));
		tuple.mesh.InitializeBlas(*rtCommandList);
		
		meshData.CreateMesh(currentName, tuple, m_SrvHeapData);

		MeshInstance meshInstance(currentName);
		MaterialInfo matInfo;

		if (mesh.material.HasTexture(AssimpMaterialType::Albedo))
		{
			auto texPath = mesh.material.GetTexture(AssimpMaterialType::Albedo).path;
			TextureInstance tex(std::wstring(texPath.begin(), texPath.end()));
			matInfo.albedo = tex.GetTextureID();
		}
		if (mesh.material.HasTexture(AssimpMaterialType::Ambient))
		{
			auto texPath = mesh.material.GetTexture(AssimpMaterialType::Ambient).path;
			TextureInstance tex(std::wstring(texPath.begin(), texPath.end()));
			

			if (MeshImport::AmbientAsMetallic & flags)
			{
				matInfo.metallic = tex.GetTextureID();
			}
			else
			{
				matInfo.ao = tex.GetTextureID();
			}
		}
		if (mesh.material.HasTexture(AssimpMaterialType::Normal))
		{
			auto texPath = mesh.material.GetTexture(AssimpMaterialType::Normal).path;
			TextureInstance tex(std::wstring(texPath.begin(), texPath.end()));
			matInfo.normal = tex.GetTextureID();
		}
		if (mesh.material.HasTexture(AssimpMaterialType::Emissive))
		{
			auto texPath = mesh.material.GetTexture(AssimpMaterialType::Emissive).path;
			TextureInstance tex(std::wstring(texPath.begin(), texPath.end()));
			matInfo.emissive = tex.GetTextureID();
		}
		if (mesh.material.HasTexture(AssimpMaterialType::Roughness))
		{
			auto texPath = mesh.material.GetTexture(AssimpMaterialType::Roughness).path;
			TextureInstance tex(std::wstring(texPath.begin(), texPath.end()));
			matInfo.roughness = tex.GetTextureID();
		}
		if (mesh.material.HasTexture(AssimpMaterialType::Metallic))
		{
			auto texPath = mesh.material.GetTexture(AssimpMaterialType::Metallic).path;
			TextureInstance tex(std::wstring(texPath.begin(), texPath.end()));
			matInfo.metallic = tex.GetTextureID();
		}
		if (mesh.material.HasTexture(AssimpMaterialType::Specular))
		{
			auto texPath = mesh.material.GetTexture(AssimpMaterialType::Specular).path;
			TextureInstance tex(std::wstring(texPath.begin(), texPath.end()));
			matInfo.specular = tex.GetTextureID();
		}
		if (mesh.material.HasTexture(AssimpMaterialType::Height))
		{
			auto texPath = mesh.material.GetTexture(AssimpMaterialType::Height).path;
			TextureInstance tex(std::wstring(texPath.begin(), texPath.end()));
			
			if (MeshImport::HeightAsNormal & flags)
			{ 
				matInfo.normal = tex.GetTextureID();
			}
			else
			{
				matInfo.height = tex.GetTextureID();
			}
		}
		if (mesh.material.HasTexture(AssimpMaterialType::Opacity))
		{
			auto texPath = mesh.material.GetTexture(AssimpMaterialType::Opacity).path;
			TextureInstance tex(std::wstring(texPath.begin(), texPath.end()));
			matInfo.opacity = tex.GetTextureID();
		}
		bool bHasAnyMat = false;
		for (size_t i = 0; i < AssimpMaterialType::Size; i++)
		{
			if(mesh.material.HasTexture((AssimpMaterialType::Type)i))
			{ 
				bHasAnyMat = true;
				break;
			}
		}

		MaterialInstance matInstance(currentName, matInfo);
		
		matInstance.SetFlags(INSTANCE_OPAQUE);

		if (matInfo.opacity != 0xffffffff)
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

		if (mesh.materialData.bHasMaterial)
		{
			Material materialData;
			materialData.diffuse = { mesh.materialData.albedo.x, mesh.materialData.albedo.y, mesh.materialData.albedo.z,  mesh.materialData.opacity };
			materialData.specular = { mesh.materialData.specular.x, mesh.materialData.specular.y, mesh.materialData.specular.z,  mesh.materialData.shininess };
			materialData.transparent = mesh.materialData.transparent;
			materialData.metallic = mesh.materialData.metallic;
			materialData.roughness = mesh.materialData.roughness;
			materialData.emissive = mesh.materialData.emissive;

			if (mesh.materialData.opacity < 1.f) 
			{ 
				matInstance.SetFlags(INSTANCE_TRANSLUCENT);

				if (MeshImport::ForceAlphaBlend & flags)
					matInstance.AddFlag(INSTANCE_ALPHA_BLEND);
				else if (MeshImport::ForceAlphaCutoff & flags)
					matInstance.AddFlag(INSTANCE_ALPHA_CUTOFF);

			}
				
			auto inst = MaterialInstance::CreateMaterial(currentName + L"/" + std::wstring(mesh.materialData.name.begin(), mesh.materialData.name.end()), materialData);
			matInstance.SetMaterial(inst);
		}

		meshInstance.SetMaterialInstance(matInstance);				
	}

	auto fenceVal1 = commandQueue->ExecuteCommandList(commandList);
	auto fenceVal2 = commandQueue->ExecuteCommandList(rtCommandList);

	commandQueue->WaitForFenceValue(fenceVal1);
	commandQueue->WaitForFenceValue(fenceVal2);

	outStaticMesh.endOffset = outStaticMesh.startOffset + num;
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

const std::wstring& MeshManager::MeshData::GetName(MeshID id)
{
	for (const auto& [name, mId] : map)
	{
		if (id == mId) return name;
	}

	return g_NoName;
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
	meshes.emplace_back(std::move(tuple));
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
		bufferSRV.StructureByteStride = sizeof(UINT);
		bufferSRV.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		desc.Buffer = bufferSRV;
		desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		device->CreateShaderResourceView(mesh.m_IndexBuffer.GetD3D12Resource().Get(), &desc, cpuHandle);
	}

	meshCreationEvent(tuple.mesh);

	AddMesh(name, tuple);
}

MeshInstanceID MeshManager::InstanceData::CreateInstance(MeshID meshID, const MeshInfo& meshInfo)
{
	const auto currentIndex = meshIds.size();
	meshIds.emplace_back(meshID);
	this->meshInfo.emplace_back(meshInfo);
	return (MeshInstanceID)currentIndex;
}
