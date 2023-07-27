#include <EnginePCH.h>
#include "Material.h"
#include <AssimpLoader.h>
#include <Application.h>
#include <TypesCompat.h>

MaterialID MaterialInstanceAccess::GetMaterialID(const MaterialInstance& materialInstance)
{
	return materialInstance.materialID;
}

MaterialInstance::MaterialInstance() 
	: materialID(MATERIAL_INVALID) 
{}

MaterialInstance::MaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor)
{
	auto materialInstance = GetMaterialManager()->LoadMaterial(name, textureIDs, materialColor);

	if (materialInstance.has_value())
		materialID = materialInstance.value().materialID;
}

MaterialInstance::MaterialInstance(const MaterialID newMaterialID) : materialID(newMaterialID)
{
	GetMaterialManager()->IncreaseRefCount(materialID);
}

MaterialInstance::MaterialInstance(const MaterialInstance& other) : materialID(other.materialID)
{
	GetMaterialManager()->IncreaseRefCount(materialID);
}

MaterialInstance& MaterialInstance::operator=(const MaterialInstance& other)
{
	if (this != &other)
	{
		GetMaterialManager()->DecreaseRefCount(materialID);
		materialID = other.materialID;
		GetMaterialManager()->IncreaseRefCount(materialID);
	}
	return *this;
}

MaterialInstance::MaterialInstance(MaterialInstance&& other) noexcept 
	: materialID(other.materialID)
{
	other.materialID = MATERIAL_INVALID;
}

MaterialInstance& MaterialInstance::operator=(MaterialInstance&& other) noexcept
{
	if (this != &other)
	{
		GetMaterialManager()->DecreaseRefCount(materialID);
		materialID = other.materialID;
		other.materialID = MATERIAL_INVALID;
	}
	return *this;
}

MaterialInstance::~MaterialInstance()
{
	GetMaterialManager()->DecreaseRefCount(materialID);
}

bool MaterialInstance::HasOpacity() const
{
	return GetMaterialManager()->HasOpacity(*this);
}

void MaterialInstance::SetFlags(const UINT flags)
{
	GetMaterialManager()->SetFlags(*this, flags);
}

void MaterialInstance::AddFlag(const UINT flag)
{
	GetMaterialManager()->AddFlags(*this, flag);
}

UINT MaterialInstance::GetCPUFlags() const
{
	return GetMaterialManager()->GetCPUFlags(*this);
}

UINT MaterialInstance::GetGPUFlags() const
{
	return GetMaterialManager()->GetGPUFlags(*this);
}

MaterialManager* MaterialInstance::GetMaterialManager() const
{
	return Application::Get().GetAssetManager()->m_MaterialManager.get();
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

		if (MeshFlags::AmbientAsMetallic & flags)
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

		if (MeshFlags::HeightAsNormal & flags)
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
	return false;
	return materialData.diffuse.z < 1.f;
}
