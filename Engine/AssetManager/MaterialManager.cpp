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

void PopulateGPUInfo(MaterialInfoGPU& gpuInfo, const TextureInstance& instance, UINT& gpuField)
{
	if (instance.IsValid())
	{
		gpuField = instance.GetTextureGPUHandle().value();
	}
}

std::optional<MaterialInfoID> MaterialManager::CreateMaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs)
{
	if (materialInfoRegistry.map.find(name) != materialInfoRegistry.map.end())
	{
		// Optional without a value indicates an error
		return std::nullopt;
	}

	const auto currentIndex = materialInfoRegistry.cpuInfo.size();
	materialInfoRegistry.cpuInfo.emplace_back(textureIDs);
	materialInfoRegistry.gpuInfo.emplace_back(MaterialInfoGPU());

	PopulateGPUInfo(materialInfoRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::albedo], materialInfoRegistry.gpuInfo[currentIndex].albedo);
	PopulateGPUInfo(materialInfoRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::normal], materialInfoRegistry.gpuInfo[currentIndex].normal);
	PopulateGPUInfo(materialInfoRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::ao], materialInfoRegistry.gpuInfo[currentIndex].ao);
	PopulateGPUInfo(materialInfoRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::emissive], materialInfoRegistry.gpuInfo[currentIndex].emissive);
	PopulateGPUInfo(materialInfoRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::roughness], materialInfoRegistry.gpuInfo[currentIndex].roughness);
	PopulateGPUInfo(materialInfoRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::specular], materialInfoRegistry.gpuInfo[currentIndex].specular);
	PopulateGPUInfo(materialInfoRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::metallic], materialInfoRegistry.gpuInfo[currentIndex].metallic);
	PopulateGPUInfo(materialInfoRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::lightmap], materialInfoRegistry.gpuInfo[currentIndex].lightmap);
	PopulateGPUInfo(materialInfoRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::opacity], materialInfoRegistry.gpuInfo[currentIndex].opacity);
	PopulateGPUInfo(materialInfoRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::height], materialInfoRegistry.gpuInfo[currentIndex].height);

	materialInfoRegistry.map[name] = currentIndex;

	return currentIndex;
}

bool MaterialManager::GetMaterialInstance(const std::wstring& name, MaterialInstance& outMaterialInstance)
{
	if (materialInfoRegistry.map.find(name) == materialInfoRegistry.map.end()) return false;
	outMaterialInstance.materialInfoID = materialInfoRegistry.map[name];
	return true;
}

MaterialInfoID MaterialManager::CreateMaterial(const std::wstring& name, const MaterialColor& material)
{
	if (materialColorRegistry.map.find(name) != materialColorRegistry.map.end()) return UINT_MAX;
	
	auto currentIndex = materialColorRegistry.materials.size();

	materialColorRegistry.materials.emplace_back(material);
	materialColorRegistry.map[name] = currentIndex;
	
	return currentIndex;
}

MaterialInfoID MaterialManager::GetMaterial(const std::wstring& name)
{
	if (materialColorRegistry.map.find(name) != materialColorRegistry.map.end()) return UINT_MAX;
	return materialColorRegistry.map[name];
}

void MaterialManager::SetMaterial(MaterialInfoID materialId, MaterialInfoID matInstanceID)
{
	//Error checking with subscript error
	materialInfoRegistry.cpuInfo[matInstanceID].materialID = materialId;
	materialInfoRegistry.gpuInfo[matInstanceID].materialID = materialId;
}

void MaterialManager::SetFlags(MaterialInfoID materialId, const UINT flags)
{
	materialInfoRegistry.gpuInfo[materialId].flags = flags;
	materialInfoRegistry.cpuInfo[materialId].flags = flags;
}

void MaterialManager::AddFlags(MaterialInfoID materialID, const UINT flags)
{
	materialInfoRegistry.gpuInfo[materialID].flags |= flags;
	materialInfoRegistry.cpuInfo[materialID].flags |= flags;
}

TextureInstance MaterialManager::GetTextureInstance(MaterialType::Type type, MaterialInfoID matInstanceId)
{
	auto& matInfo = materialInfoRegistry.cpuInfo[matInstanceId];
	return matInfo.textures[type];
}

const std::wstring& MaterialManager::GetMaterialInstanceName(MaterialInfoID matInstanceId) const
{
	if (matInstanceId == UINT_MAX) return g_NoMaterial;

	for (const auto& [name, id] : materialInfoRegistry.map)
	{
		if (id == matInstanceId) return name;
	}

	return g_NoMaterial;
}

const std::wstring& MaterialManager::GetMaterialName(MaterialInfoID matInstanceId) const
{
	auto matId = materialInfoRegistry.cpuInfo[matInstanceId].materialID;

	if (matId == UINT_MAX) return g_NoMaterial;

	for (const auto& [name, id] : materialColorRegistry.map)
	{
		if (id == matInstanceId) return name;
	}

	return g_NoMaterial;
}

MaterialColor& MaterialManager::GetUserDefinedMaterial(MaterialInfoID matInstanceId)
{
	auto materialID = materialInfoRegistry.cpuInfo[matInstanceId].materialID;
	
	if (materialID == UINT_MAX)
	{
		return g_NoUserMaterial;
	}

	return materialColorRegistry.materials[materialID];
}