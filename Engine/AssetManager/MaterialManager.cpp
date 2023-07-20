#include <EnginePCH.h>
#include <MaterialManager.h>
#include <TextureManager.h>
#include <Texture.h>

std::wstring g_NoMaterial = L"No Material";
MaterialColor g_NoUserMaterial; //Already initialized with values

void PopulateGPUInfo(MaterialInfoGPU& gpuInfo, const TextureInstance& instance, UINT& gpuField)
{
	if (instance.IsValid())
	{
		gpuField = instance.GetTextureGPUHandle().value();
	}
}

const MaterialInstance& MaterialManager::LoadMaterial(const std::wstring& name, const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor)
{
	if (materialRegistry.map.find(name) != materialRegistry.map.end())
	{
		return materialRegistry.map[name];
	}

	return CreateMaterial(name, textureIDs, materialColor);
}

const MaterialInstance& MaterialManager::CreateMaterial(const std::wstring& name, const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor)
{
	MaterialID currentIndex = 0;

	if (!releasedMaterialIDs.empty())
	{
		currentIndex = releasedMaterialIDs.front();
		releasedMaterialIDs.erase(releasedMaterialIDs.begin());
	}
	else
	{
		currentIndex = materialRegistry.cpuInfo.size(),

		materialRegistry.cpuInfo.emplace_back(textureIDs);
		materialRegistry.gpuInfo.emplace_back(MaterialInfoGPU());
		materialRegistry.materialColors.emplace_back(materialColor);
		materialRegistry.refCounts[currentIndex].store(0U);
	}
	
	assert(materialRegistry.cpuInfo.size() == materialRegistry.gpuInfo.size());
	assert(materialRegistry.gpuInfo.size() == materialRegistry.materialColors.size());

	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::albedo], materialRegistry.gpuInfo[currentIndex].albedo);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::normal], materialRegistry.gpuInfo[currentIndex].normal);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::ao], materialRegistry.gpuInfo[currentIndex].ao);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::emissive], materialRegistry.gpuInfo[currentIndex].emissive);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::roughness], materialRegistry.gpuInfo[currentIndex].roughness);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::specular], materialRegistry.gpuInfo[currentIndex].specular);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::metallic], materialRegistry.gpuInfo[currentIndex].metallic);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::lightmap], materialRegistry.gpuInfo[currentIndex].lightmap);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::opacity], materialRegistry.gpuInfo[currentIndex].opacity);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::height], materialRegistry.gpuInfo[currentIndex].height);

	materialRegistry.map[name] = currentIndex;

	return materialRegistry.map[name];
}

bool MaterialManager::IsMaterialValid(const MaterialInstance& materialInstance) const
{
	if (materialInstance.materialID == MATERIAL_INVALID)
		return false;

	if (materialInstance.materialID >= materialRegistry.cpuInfo.size())
		return false;

	return true;
}

void MaterialManager::IncreaseRefCount(const MaterialID materialID)
{
	if (IsMaterialValid(materialID))
	{
		materialRegistry.refCounts[materialID].fetch_add(1, std::memory_order_relaxed);
	}
}

void MaterialManager::DecreaseRefCount(const MaterialID materialID)
{
	if (IsMaterialValid(materialID))
	{
		if (materialRegistry.refCounts[materialID].fetch_sub(1, std::memory_order_relaxed) == 1)
		{
			ReleaseMaterial(materialID);
		}
	}
}

void MaterialManager::ReleaseMaterial(const MaterialID materialID)
{
	materialRegistry.cpuInfo[materialID] = {}; // Reset to default
	materialRegistry.gpuInfo[materialID] = {};
	materialRegistry.materialColors[materialID] = {};

	for (auto it = materialRegistry.map.begin(); it != materialRegistry.map.end();)
	{
		if (it->second.materialID == materialID)
		{
			it = materialRegistry.map.erase(it);
		}
		else
		{
			++it;
		}
	}

	releasedMaterialIDs.emplace_back(materialID);
}

std::optional<MaterialInstance> MaterialManager::GetMaterialInstance(const std::wstring& name)
{
	if (materialRegistry.map.find(name) == materialRegistry.map.end()) 
		return std::nullopt;

	return materialRegistry.map[name];
}

void MaterialManager::SetFlags(const MaterialInstance& materialInstance, const UINT flags)
{
	materialRegistry.gpuInfo[materialInstance.materialID].flags = flags;
	materialRegistry.cpuInfo[materialInstance.materialID].flags = flags;
}

void MaterialManager::AddFlags(const MaterialInstance& materialInstance, const UINT flags)
{
	materialRegistry.gpuInfo[materialInstance.materialID].flags |= flags;
	materialRegistry.cpuInfo[materialInstance.materialID].flags |= flags;
}

TextureInstance MaterialManager::GetTextureInstance(const MaterialInstance& materialInstance, MaterialType::Type type)
{
	auto& matInfo = materialRegistry.cpuInfo[materialInstance.materialID];
	return matInfo.textures[type];
}

const std::wstring& MaterialManager::GetMaterialName(const MaterialInstance& materialInstance) const
{
	if (materialInstance.materialID == MATERIAL_INVALID)
		return g_NoMaterial;

	for (const auto& [name, id] : materialRegistry.map)
	{
		if (id.materialID == materialInstance.materialID)
			return name;
	}

	return g_NoMaterial;
}

const MaterialColor& MaterialManager::GetMaterialColor(const MaterialInstance& materialInstance)
{
	if (materialInstance.materialID == MATERIAL_INVALID)
	{
		return g_NoUserMaterial;
	}

	return materialRegistry.materialColors[materialInstance.materialID];
}

const std::vector<MaterialInfoCPU>& MaterialManager::GetMaterialCPUInfoData() const
{
	return materialRegistry.cpuInfo;
}

const std::vector<MaterialInfoGPU>& MaterialManager::GetMaterialGPUInfoData() const
{
	return materialRegistry.gpuInfo;
}

const std::vector<MaterialColor>& MaterialManager::GetMaterialColorData() const
{
	return materialRegistry.materialColors;
}