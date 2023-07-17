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

	if (IsMaterialValid(textureIDs.albedo.textureID))
	{
		instanceData.gpuInfo[currentIndex].albedo = textureData.textures[textureIDs.albedo.textureID].heapID;
	}
	if (IsMaterialValid(textureIDs.normal.textureID))
	{
		instanceData.gpuInfo[currentIndex].normal = textureData.textures[textureIDs.normal.textureID].heapID;
	}
	if (IsMaterialValid(textureIDs.ao.textureID))
	{
		instanceData.gpuInfo[currentIndex].ao = textureData.textures[textureIDs.ao.textureID].heapID;
	}
	if (IsMaterialValid(textureIDs.emissive.textureID))
	{
		instanceData.gpuInfo[currentIndex].emissive = textureData.textures[textureIDs.emissive.textureID].heapID;
	}
	if (IsMaterialValid(textureIDs.roughness.textureID))
	{
		instanceData.gpuInfo[currentIndex].roughness = textureData.textures[textureIDs.roughness.textureID].heapID;
	}
	if (IsMaterialValid(textureIDs.specular.textureID))
	{
		instanceData.gpuInfo[currentIndex].specular = textureData.textures[textureIDs.specular.textureID].heapID;
	}
	if (IsMaterialValid(textureIDs.metallic.textureID))
	{
		instanceData.gpuInfo[currentIndex].metallic = textureData.textures[textureIDs.metallic.textureID].heapID;
	}
	if (IsMaterialValid(textureIDs.lightmap.textureID))
	{
		instanceData.gpuInfo[currentIndex].lightmap = textureData.textures[textureIDs.lightmap.textureID].heapID;
	}
	if (IsMaterialValid(textureIDs.opacity.textureID))
	{
		instanceData.gpuInfo[currentIndex].opacity = textureData.textures[textureIDs.opacity.textureID].heapID;
	}
	if (IsMaterialValid(textureIDs.height.textureID))
	{
		instanceData.gpuInfo[currentIndex].height = textureData.textures[textureIDs.height.textureID].heapID;
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

TextureInstance MaterialManager::GetTextureInstance(MaterialType::Type type, MaterialInstanceID matInstanceId)
{
	auto& matInfo = instanceData.cpuInfo[matInstanceId];

	switch (type)
	{
	case MaterialType::ao:
		return matInfo.ao;
	case MaterialType::albedo:
		return matInfo.albedo;
	case MaterialType::normal:
		return matInfo.normal;
	case MaterialType::roughness:
		return matInfo.roughness;
	case MaterialType::metallic:
		return matInfo.metallic;
	case MaterialType::opacity:
		return matInfo.opacity;
	case MaterialType::emissive:
		return matInfo.emissive;
	case MaterialType::lightmap:
		return matInfo.lightmap;
	case MaterialType::height:
		return matInfo.height;
	default:
		return TextureInstance();
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