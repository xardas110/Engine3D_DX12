#include <EnginePCH.h>
#include <MeshManager.h>
#include <AssetManager.h>
#include <Application.h>
#include <CommandList.h>
#include <CommandQueue.h>
#include <AssimpLoader.h>
#include <Material.h>
#include <Logger.h>

/**
 * Applies alpha flags to a material instance if applicable.
 *
 * @param materialInstance Reference to the material instance being modified.
 * @param importFlags Flags that specify how the material instance should be modified.
 */
void ApplyAlphaFlags(MaterialInstance& materialInstance, MeshImport::Flags importFlags)
{
	// If alpha blending is forced by the import flags, apply alpha blending
	if (importFlags & MeshImport::ForceAlphaBlend)
	{
		materialInstance.AddFlag(INSTANCE_ALPHA_BLEND);
	}
	// If alpha cutoff is forced by the import flags, apply alpha cutoff
	else if (importFlags & MeshImport::ForceAlphaCutoff)
	{
		materialInstance.AddFlag(INSTANCE_ALPHA_CUTOFF);
	}
}

/**
 * Applies transparency to a material instance if applicable.
 *
 * @param materialInstance Reference to the material instance being modified.
 * @param importFlags Flags that specify how the material instance should be modified.
 * @param materialInfo Information about the material.
 */
void ApplyTransparency(MaterialInstance& materialInstance, MeshImport::Flags importFlags, MaterialInfoCPU materialInfo)
{
	if (materialInfo.GetTexture(MaterialType::albedo).IsValid())
	{
		// Apply translucency flag to the material instance
		materialInstance.SetFlags(INSTANCE_TRANSLUCENT);

		// Apply appropriate alpha blending or cutoff based on the import flags
		ApplyAlphaFlags(materialInstance, importFlags);
	}
}

/**
 * Applies import flags to a material instance.
 *
 * @param materialInstance Reference to the material instance being modified.
 * @param importFlags Flags that specify how the material instance should be modified.
 */
void ApplySpecificImportFlags(MaterialInstance& materialInstance, MeshImport::Flags importFlags)
{
	// Apply Ambient Occlusion, Roughness, and Metalness as Specular Texture if specified in import flags
	if (importFlags & MeshImport::AO_Rough_Metal_As_Spec_Tex)
	{
		materialInstance.AddFlag(MATERIAL_FLAG_AO_ROUGH_METAL_AS_SPECULAR);
	}

	// Apply Base Color Alpha if specified in import flags
	if (importFlags & MeshImport::BaseColorAlpha)
	{
		materialInstance.AddFlag(MATERIAL_FLAG_BASECOLOR_ALPHA);
	}

	// Invert Material Normals if specified in import flags
	if (importFlags & MeshImport::InvertMaterialNormals)
	{
		materialInstance.AddFlag(MATERIAL_FLAG_INVERT_NORMALS);
	}
}

/**
 * Applies import flags to a material instance.
 *
 * @param materialInstance Reference to the material instance being modified.
 * @param importFlags Flags that specify how the material instance should be modified.
 * @param materialInfo Information about the material.
 */
void ApplyMaterialInstanceImportFlags(MaterialInstance& materialInstance, MeshImport::Flags importFlags, MaterialInfoCPU materialInfo)
{
	// Set default flags for the material instance
	materialInstance.SetFlags(INSTANCE_OPAQUE);

	// Check and apply transparency if required
	ApplyTransparency(materialInstance, importFlags, materialInfo);

	// Apply specific import flags based on the provided importFlags
	ApplySpecificImportFlags(materialInstance, importFlags);
}

MeshManager::MeshManager(const SRVHeapData& srvHeapData)
	:m_SrvHeapData(srvHeapData)
{}

void MeshManager::LoadStaticMesh(CommandList& commandList, std::shared_ptr<CommandList> rtCommandList, const std::string& path, StaticMeshInstance& outStaticMesh, MeshImport::Flags flags)
{
    AssimpLoader loader(path, flags);
    const auto& sm = loader.GetAssimpStaticMesh();
    LOG_INFO("Num meshes in static mesh: %i", (int)sm.meshes.size());

    outStaticMesh.startOffset = instanceData.meshInfo.size();

    for (size_t i = 0; i < sm.meshes.size(); i++)
    {
		auto mesh = sm.meshes[i];

        const std::wstring currentName = std::wstring(path.begin(), path.end()) + L"/" + std::to_wstring(i) + L"/" + std::wstring(mesh.name.begin(), mesh.name.end());

        auto internalMesh = Mesh::CreateMesh(commandList, rtCommandList, mesh.vertices, mesh.indices, flags & MeshImport::RHCoords, flags & MeshImport::CustomTangent);
        meshData.CreateMesh(currentName, std::move(internalMesh), m_SrvHeapData);

		MaterialColor materialData = MaterialHelper::CreateMaterial(mesh.materialData);

        MaterialInfoCPU matInfo = MaterialInfoHelper::PopulateMaterialInfo(mesh, flags);
        MaterialInstance matInstance(currentName, matInfo, materialData);
        ApplyMaterialInstanceImportFlags(matInstance, flags, matInfo);

        if (mesh.materialData.bHasMaterial)
        {
            if (MaterialHelper::IsTransparent(materialData))
            {
                ApplyTransparency(matInstance, flags, matInfo);
            }
        }

        MeshInstance meshInstance(currentName);
        meshInstance.SetMaterialInstance(matInstance);
    }

    outStaticMesh.endOffset = outStaticMesh.startOffset + sm.meshes.size();
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

	return L"No Name";
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
