#include <EnginePCH.h>
#include "Material.h"
#include <AssimpLoader.h>
#include <Application.h>
#include <TypesCompat.h>

MaterialManager* MaterialInstance::GetMaterialManager() const 
{
	return Application::Get().GetAssetManager()->m_MaterialManager.get();
}

MaterialID MaterialInstance::CreateMaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs)
{
	return Application::Get().GetAssetManager()->m_MaterialManager->CreateMaterialInstance(name, textureIDs).value();
}

MaterialInstance::MaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs)
{
	materialID = MaterialInstance::CreateMaterialInstance(name, textureIDs);
}

bool MaterialInstance::GetMaterialInstance(const std::wstring& name)
{
	return GetMaterialManager()->GetMaterialInstance(name, *this);
}

MaterialID MaterialInstance::CreateMaterial(const std::wstring& name, const MaterialColor& material)
{
	return Application::Get().GetAssetManager()->m_MaterialManager->CreateMaterial(name, material);
}

void MaterialInstance::SetMaterial(const MaterialID materialId)
{
	GetMaterialManager()->SetMaterial(materialID, materialId);
}

void MaterialInstance::SetFlags(const UINT flags)
{
	GetMaterialManager()->SetFlags(materialID, flags);
}

void MaterialInstance::AddFlag(const UINT flag)
{
	GetMaterialManager()->AddFlags(materialID, flag);
}

UINT MaterialInstance::GetCPUFlags() const
{
	return GetMaterialManager()->materialInfoRegistry.cpuInfo[materialID].flags;
}

UINT MaterialInstance::GetGPUFlags() const
{
	return GetMaterialManager()->materialInfoRegistry.gpuInfo[materialID].flags;
}

MaterialInfoCPU MaterialInfoHelper::PopulateMaterialInfo(const AssimpMesh& mesh, int flags)
{
	MaterialInfoCPU matInfo;

	if (mesh.material.HasTexture(AssimpMaterialType::Albedo))
	{
		const auto texPath = mesh.material.GetTexture(AssimpMaterialType::Albedo).path;
		matInfo.textures[MaterialType::albedo] = TextureInstance(std::wstring(texPath.begin(), texPath.end()));
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Ambient))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Ambient).path;
		TextureInstance tex(std::wstring(texPath.begin(), texPath.end()));

		if (MeshImport::AmbientAsMetallic & flags)
		{
			matInfo.textures[MaterialType::metallic] = tex;
		}
		else
		{
			matInfo.textures[MaterialType::ao] = tex;
		}
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Normal))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Normal).path;		
		matInfo.textures[MaterialType::normal] = TextureInstance(std::wstring(texPath.begin(), texPath.end()));;
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Emissive))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Emissive).path;
		matInfo.textures[MaterialType::emissive] = TextureInstance(std::wstring(texPath.begin(), texPath.end()));
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Roughness))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Roughness).path;		
		matInfo.textures[MaterialType::roughness] = TextureInstance(std::wstring(texPath.begin(), texPath.end()));
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Metallic))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Metallic).path;		
		matInfo.textures[MaterialType::metallic] = TextureInstance(std::wstring(texPath.begin(), texPath.end()));
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Specular))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Specular).path;		
		matInfo.textures[MaterialType::specular] = TextureInstance(std::wstring(texPath.begin(), texPath.end()));
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Height))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Height).path;
		TextureInstance tex(std::wstring(texPath.begin(), texPath.end()));

		if (MeshImport::HeightAsNormal & flags)
		{
			matInfo.textures[MaterialType::normal] = tex;
		}
		else
		{
			matInfo.textures[MaterialType::height] = tex;
		}
	}
	if (mesh.material.HasTexture(AssimpMaterialType::Opacity))
	{
		auto texPath = mesh.material.GetTexture(AssimpMaterialType::Opacity).path;	
		matInfo.textures[MaterialType::opacity] = TextureInstance(std::wstring(texPath.begin(), texPath.end()));
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
