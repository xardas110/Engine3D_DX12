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

std::optional<MaterialID> MaterialManager::CreateMaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs)
{
	if (instanceData.map.find(name) != instanceData.map.end())
	{
		// Optional without a value indicates an error
		return std::nullopt;
	}

	const auto currentIndex = instanceData.cpuInfo.size();
	instanceData.cpuInfo.emplace_back(textureIDs);
	instanceData.gpuInfo.emplace_back(MaterialInfoGPU());

	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.textures[MaterialType::albedo], instanceData.gpuInfo[currentIndex].albedo);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.textures[MaterialType::normal], instanceData.gpuInfo[currentIndex].normal);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.textures[MaterialType::ao], instanceData.gpuInfo[currentIndex].ao);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.textures[MaterialType::emissive], instanceData.gpuInfo[currentIndex].emissive);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.textures[MaterialType::roughness], instanceData.gpuInfo[currentIndex].roughness);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.textures[MaterialType::specular], instanceData.gpuInfo[currentIndex].specular);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.textures[MaterialType::metallic], instanceData.gpuInfo[currentIndex].metallic);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.textures[MaterialType::lightmap], instanceData.gpuInfo[currentIndex].lightmap);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.textures[MaterialType::opacity], instanceData.gpuInfo[currentIndex].opacity);
	PopulateGPUInfo(instanceData.gpuInfo[currentIndex], textureIDs.textures[MaterialType::height], instanceData.gpuInfo[currentIndex].height);

	instanceData.map[name] = currentIndex;

	return currentIndex;
}

bool MaterialManager::GetMaterialInstance(const std::wstring& name, MaterialInstance& outMaterialInstance)
{
	assert(instanceData.map.find(name) != instanceData.map.end() && "Unable to find material instance");

	if (instanceData.map.find(name) == instanceData.map.end()) return false;

	outMaterialInstance.materialID = instanceData.map[name];

	return true;
}

MaterialID MaterialManager::CreateMaterial(const std::wstring& name, const MaterialColor& material)
{
	if (materialColorRegistry.map.find(name) != materialColorRegistry.map.end()) return UINT_MAX;
	
	auto currentIndex = materialColorRegistry.materials.size();

	materialColorRegistry.materials.emplace_back(material);
	materialColorRegistry.map[name] = currentIndex;
	
	return currentIndex;
}

MaterialID MaterialManager::GetMaterial(const std::wstring& name)
{
	if (materialColorRegistry.map.find(name) != materialColorRegistry.map.end()) return UINT_MAX;
	return materialColorRegistry.map[name];
}

void MaterialManager::SetMaterial(MaterialID materialId, MaterialID matInstanceID)
{
	//Error checking with subscript error
	instanceData.cpuInfo[matInstanceID].materialID = materialId;
	instanceData.gpuInfo[matInstanceID].materialID = materialId;
}

void MaterialManager::SetFlags(MaterialID materialId, const UINT flags)
{
	instanceData.gpuInfo[materialId].flags = flags;
	instanceData.cpuInfo[materialId].flags = flags;
}

void MaterialManager::AddFlags(MaterialID materialID, const UINT flags)
{
	instanceData.gpuInfo[materialID].flags |= flags;
	instanceData.cpuInfo[materialID].flags |= flags;
}

TextureInstance MaterialManager::GetTextureInstance(MaterialType::Type type, MaterialID matInstanceId)
{
	auto& matInfo = instanceData.cpuInfo[matInstanceId];
	return matInfo.textures[type];
}

const std::wstring& MaterialManager::GetMaterialInstanceName(MaterialID matInstanceId) const
{
	if (matInstanceId == UINT_MAX) return g_NoMaterial;

	for (const auto& [name, id] : instanceData.map)
	{
		if (id == matInstanceId) return name;
	}

	return g_NoMaterial;
}

const std::wstring& MaterialManager::GetMaterialName(MaterialID matInstanceId) const
{
	auto matId = instanceData.cpuInfo[matInstanceId].materialID;

	if (matId == UINT_MAX) return g_NoMaterial;

	for (const auto& [name, id] : materialColorRegistry.map)
	{
		if (id == matInstanceId) return name;
	}

	return g_NoMaterial;
}

MaterialColor& MaterialManager::GetUserDefinedMaterial(MaterialID matInstanceId)
{
	auto materialID = instanceData.cpuInfo[matInstanceId].materialID;
	
	if (materialID == UINT_MAX)
	{
		return g_NoUserMaterial;
	}

	return materialColorRegistry.materials[materialID];
}