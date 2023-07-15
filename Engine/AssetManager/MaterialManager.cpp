#include <EnginePCH.h>
#include <MaterialManager.h>
#include <TextureManager.h>
#include <Texture.h>

std::wstring g_NoMaterial = L"No Material";
Material g_NoUserMaterial; //Already initialized with values

bool IsMaterialValid(UINT id)
{
	return id != UINT_MAX;
}

MaterialManager::MaterialManager(const TextureManager& textureManager)
	:m_TextureManager(textureManager)
{}

MaterialInstanceID MaterialManager::CreateMaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs)
{
	assert(instanceData.map.find(name) == instanceData.map.end() && "Material instance exists!");
	if (instanceData.map.find(name) != instanceData.map.end()) return UINT_MAX;

	const auto currentIndex = instanceData.cpuInfo.size();
	instanceData.cpuInfo.emplace_back(textureIDs);
	instanceData.gpuInfo.emplace_back(MaterialInfoGPU());
	instanceData.refCounter.emplace_back(UINT(1U));

	const auto& textureData = m_TextureManager.textureData;

	if (IsMaterialValid(textureIDs.albedo.GetTextureID()))
	{
		instanceData.gpuInfo[currentIndex].albedo = textureData.textures[textureIDs.albedo.GetTextureID()].heapID;
	}
	if (IsMaterialValid(textureIDs.normal.GetTextureID()))
	{
		instanceData.gpuInfo[currentIndex].normal = textureData.textures[textureIDs.normal.GetTextureID()].heapID;
	}
	if (IsMaterialValid(textureIDs.ao.GetTextureID()))
	{
		instanceData.gpuInfo[currentIndex].ao = textureData.textures[textureIDs.ao.GetTextureID()].heapID;
	}
	if (IsMaterialValid(textureIDs.emissive.GetTextureID()))
	{
		instanceData.gpuInfo[currentIndex].emissive = textureData.textures[textureIDs.emissive.GetTextureID()].heapID;
	}
	if (IsMaterialValid(textureIDs.roughness.GetTextureID()))
	{
		instanceData.gpuInfo[currentIndex].roughness = textureData.textures[textureIDs.roughness.GetTextureID()].heapID;
	}
	if (IsMaterialValid(textureIDs.specular.GetTextureID()))
	{
		instanceData.gpuInfo[currentIndex].specular = textureData.textures[textureIDs.specular.GetTextureID()].heapID;
	}
	if (IsMaterialValid(textureIDs.metallic.GetTextureID()))
	{
		instanceData.gpuInfo[currentIndex].metallic = textureData.textures[textureIDs.metallic.GetTextureID()].heapID;
	}
	if (IsMaterialValid(textureIDs.lightmap.GetTextureID()))
	{
		instanceData.gpuInfo[currentIndex].lightmap = textureData.textures[textureIDs.lightmap.GetTextureID()].heapID;
	}
	if (IsMaterialValid(textureIDs.opacity.GetTextureID()))
	{
		instanceData.gpuInfo[currentIndex].opacity = textureData.textures[textureIDs.opacity.GetTextureID()].heapID;
	}
	if (IsMaterialValid(textureIDs.height.GetTextureID()))
	{
		instanceData.gpuInfo[currentIndex].height = textureData.textures[textureIDs.height.GetTextureID()].heapID;
	}

	instanceData.map[name] = currentIndex;

	return currentIndex;
}

bool MaterialManager::GetMaterialInstance(const std::wstring& name, MaterialInstance& outMaterialInstance)
{
	assert(instanceData.map.find(name) != instanceData.map.end() && "Unable to find material instance");

	if (instanceData.map.find(name) == instanceData.map.end()) return false;

	outMaterialInstance.materialInstanceID = instanceData.map[name];

	return true;
}

MaterialID MaterialManager::CreateMaterial(const std::wstring& name, const Material& material)
{
	assert(materialData.map.find(name) == materialData.map.end());
	if (materialData.map.find(name) != materialData.map.end()) return UINT_MAX;
	
	auto currentIndex = materialData.materials.size();

	materialData.materials.emplace_back(material);
	materialData.map[name] = currentIndex;
	
	return currentIndex;
}

MaterialID MaterialManager::GetMaterial(const std::wstring& name)
{
	assert(materialData.map.find(name) == materialData.map.end());
	if (materialData.map.find(name) != materialData.map.end()) return UINT_MAX;

	return materialData.map[name];
}

void MaterialManager::SetMaterial(MaterialID materialId, MaterialInstanceID matInstanceID)
{
	//Error checking with subscript error
	instanceData.cpuInfo[matInstanceID].materialID = materialId;
	instanceData.gpuInfo[matInstanceID].materialID = materialId;
}

TextureID MaterialManager::GetTextureID(MaterialType::Type type, MaterialInstanceID matInstanceId)
{
	auto& matInfo = instanceData.cpuInfo[matInstanceId];

	switch (type)
	{
	case MaterialType::ao:
		return matInfo.ao.GetTextureID();
	case MaterialType::albedo:
		return matInfo.albedo.GetTextureID();
	case MaterialType::normal:
		return matInfo.normal.GetTextureID();
	case MaterialType::roughness:
		return matInfo.roughness.GetTextureID();
	case MaterialType::metallic:
		return matInfo.metallic.GetTextureID();
	case MaterialType::opacity:
		return matInfo.opacity.GetTextureID();
	case MaterialType::emissive:
		return matInfo.emissive.GetTextureID();
	case MaterialType::lightmap:
		return matInfo.lightmap.GetTextureID();
	case MaterialType::height:
		return matInfo.height.GetTextureID();
	default:
		return UINT_MAX;
		break;
	}
}

const std::wstring& MaterialManager::GetMaterialInstanceName(MaterialInstanceID matInstanceId) const
{
	if (matInstanceId == UINT_MAX) return g_NoMaterial;

	for (const auto& [name, id] : instanceData.map)
	{
		if (id == matInstanceId) return name;
	}

	return g_NoMaterial;
}

const std::wstring& MaterialManager::GetMaterialName(MaterialInstanceID matInstanceId) const
{
	auto matId = instanceData.cpuInfo[matInstanceId].materialID;

	if (matId == UINT_MAX) return g_NoMaterial;

	for (const auto& [name, id] : materialData.map)
	{
		if (id == matInstanceId) return name;
	}

	return g_NoMaterial;
}

Material& MaterialManager::GetUserDefinedMaterial(MaterialInstanceID matInstanceId)
{
	auto materialID = instanceData.cpuInfo[matInstanceId].materialID;
	
	if (materialID == UINT_MAX)
	{
		return g_NoUserMaterial;
	}

	return materialData.materials[materialID];
}