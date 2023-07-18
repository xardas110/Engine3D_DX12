#include <EnginePCH.h>
#include <MaterialManager.h>
#include <TextureManager.h>
#include <Texture.h>

std::wstring g_NoMaterial = L"No Material";
MaterialColor g_NoUserMaterial; //Already initialized with values

bool IsMaterialValid(UINT id)
{
	return id != UINT_MAX;
}

MaterialManager::MaterialManager(const TextureManager& textureManager)
	:m_TextureManager(textureManager)
{}

void PopulateGPUInfo(MaterialInfoGPU& gpuInfo, const TextureInstance& instance, UINT& gpuField)
{
	if (instance.IsValid())
	{
		gpuField = instance.GetTextureGPUHandle().value();
	}
}

std::optional<MaterialInstanceID> MaterialManager::CreateMaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs)
{
	if (instanceData.map.find(name) != instanceData.map.end())
	{
		// Optional without a value indicates an error
		return std::nullopt;
	}

	const auto currentIndex = instanceData.cpuInfo.size();
	instanceData.cpuInfo.emplace_back(textureIDs);
	instanceData.gpuInfo.emplace_back(MaterialInfoGPU());

	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.albedo, instanceData.gpuInfo[currentIndex].albedo);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.normal, instanceData.gpuInfo[currentIndex].normal);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.ao, instanceData.gpuInfo[currentIndex].ao);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.emissive, instanceData.gpuInfo[currentIndex].emissive);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.roughness, instanceData.gpuInfo[currentIndex].roughness);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.specular, instanceData.gpuInfo[currentIndex].specular);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.metallic, instanceData.gpuInfo[currentIndex].metallic);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.lightmap, instanceData.gpuInfo[currentIndex].lightmap);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.opacity, instanceData.gpuInfo[currentIndex].opacity);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.height, instanceData.gpuInfo[currentIndex].height);

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

MaterialID MaterialManager::CreateMaterial(const std::wstring& name, const MaterialColor& material)
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

MaterialColor& MaterialManager::GetUserDefinedMaterial(MaterialInstanceID matInstanceId)
{
	auto materialID = instanceData.cpuInfo[matInstanceId].materialID;
	
	if (materialID == UINT_MAX)
	{
		return g_NoUserMaterial;
	}

	return materialData.materials[materialID];
}