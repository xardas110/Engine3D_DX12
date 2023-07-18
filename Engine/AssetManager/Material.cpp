#include <EnginePCH.h>
#include "Material.h"
#include <AssimpLoader.h>
#include <Application.h>
#include <TypesCompat.h>

MaterialInstanceID MaterialInstance::CreateMaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs)
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;
	materialInstanceID = materialManager->CreateMaterialInstance(name, textureIDs).value();

	return materialInstanceID;
}

bool MaterialInstance::GetMaterialInstance(const std::wstring& name)
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;

	return materialManager->GetMaterialInstance(name, *this);
}

MaterialID MaterialInstance::CreateMaterial(const std::wstring& name, const MaterialColor& material)
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;
	return materialManager->CreateMaterial(name, material);
}

void MaterialInstance::SetMaterial(MaterialID materialId)
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;
	materialManager->SetMaterial(materialInstanceID, materialId);
}

void MaterialInstance::SetFlags(UINT flags)
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;
	materialManager->instanceData.gpuInfo[materialInstanceID].flags = flags;
	materialManager->instanceData.cpuInfo[materialInstanceID].flags = flags;
}

void MaterialInstance::AddFlag(UINT flag)
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;
	materialManager->instanceData.gpuInfo[materialInstanceID].flags |= flag;
	materialManager->instanceData.cpuInfo[materialInstanceID].flags |= flag;
}

UINT MaterialInstance::GetCPUFlags()
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;
	return materialManager->instanceData.cpuInfo[materialInstanceID].flags;
}

UINT MaterialInstance::GetGPUFlags()
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;
	return materialManager->instanceData.gpuInfo[materialInstanceID].flags;
}

MaterialInfoCPU MaterialInfoHelper::PopulateMaterialInfo(const AssimpMesh& mesh, int flags)
{
	MaterialInfoCPU matInfo;

	if (mesh.material.HasTexture(AssimpMaterialType::Albedo))
	{
		const auto texPath = mesh.material.GetTexture(AssimpMaterialType::Albedo).path;
		matInfo.albedo = TextureInstance(std::wstring(texPath.begin(), texPath.end()));
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Ambient))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Ambient).path;
		TextureInstance tex(std::wstring(texPath.begin(), texPath.end()));

		if (MeshImport::AmbientAsMetallic & flags)
		{
			matInfo.metallic = tex;
		}
		else
		{
			matInfo.ao = tex;
		}
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Normal))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Normal).path;		
		matInfo.normal = TextureInstance(std::wstring(texPath.begin(), texPath.end()));;
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Emissive))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Emissive).path;
		matInfo.emissive = TextureInstance(std::wstring(texPath.begin(), texPath.end()));
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Roughness))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Roughness).path;		
		matInfo.roughness = TextureInstance(std::wstring(texPath.begin(), texPath.end()));
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Metallic))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Metallic).path;		
		matInfo.metallic = TextureInstance(std::wstring(texPath.begin(), texPath.end()));
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Specular))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Specular).path;		
		matInfo.specular = TextureInstance(std::wstring(texPath.begin(), texPath.end()));
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Height))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Height).path;
		TextureInstance tex(std::wstring(texPath.begin(), texPath.end()));

		if (MeshImport::HeightAsNormal & flags)
		{
			matInfo.normal = tex;
		}
		else
		{
			matInfo.height = tex;
		}
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Opacity))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Opacity).path;	
		matInfo.opacity = TextureInstance(std::wstring(texPath.begin(), texPath.end()));
	}
	bool bHasAnyMat = false;
	for (size_t i = 0; i < AssimpMaterialType::Size; i++)
	{
		if (mesh.material.HasTexture((AssimpMaterialType::Type)i))
		{
			bHasAnyMat = true;
			break;
		}
	}

	return matInfo;
}

bool MaterialInfoHelper::IsTextureValid(UINT texture)
{
	return texture != UINT_MAX;
}

MaterialColor MaterialHelper::CreateMaterial(const AssimpMaterialData& materialData)
{
	MaterialColor material;

	material.diffuse = {
	materialData.albedo.x,
	materialData.albedo.y,
	materialData.albedo.z,
	materialData.opacity };

	material.specular = {
		materialData.specular.x,
		materialData.specular.y,
		materialData.specular.z,
		materialData.shininess };

	material.transparent = materialData.transparent;
	material.metallic = materialData.metallic;
	material.roughness = materialData.roughness;
	material.emissive = materialData.emissive;

	return material;
}

bool MaterialHelper::IsTransparent(const struct MaterialColor& materialData)
{
	return materialData.diffuse.z < 1.f;
}