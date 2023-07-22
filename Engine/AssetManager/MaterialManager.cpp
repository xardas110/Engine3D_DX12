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
	// Shared mutex for materialinstance map lookup
	{
		SHARED_LOCK(MaterialRegistryMap, materialRegistry.mapMutex);
		if (materialRegistry.map.find(name) != materialRegistry.map.end())
		{
			return materialRegistry.map[name];
		}
	}

	return CreateMaterial(name, textureIDs, materialColor);
}

const MaterialInstance& MaterialManager::CreateMaterial(const std::wstring& name, const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor)
{
	MaterialID currentIndex = 0;

	// Lock the registry mutexes for write operations.
	{
		SCOPED_UNIQUE_LOCK(
			releasedMaterialIDsMutex, 
			materialRegistry.cpuInfoMutex, 
			materialRegistry.gpuInfoMutex, 
			materialRegistry.materialColorsMutex);

		if (!releasedMaterialIDs.empty())
		{
			currentIndex = releasedMaterialIDs.front();
			releasedMaterialIDs.erase(releasedMaterialIDs.begin());
		}
		else
		{
			currentIndex = materialRegistry.cpuInfo.size(),
			assert(currentIndex < MATERIAL_MANAGER_MAX_MATERIALS && "Material Manager exceeds maximum materials");

			materialRegistry.cpuInfo.emplace_back(textureIDs);
			materialRegistry.gpuInfo.emplace_back(MaterialInfoGPU());
			materialRegistry.materialColors.emplace_back(materialColor);
			materialRegistry.refCounts[currentIndex].store(0U);
		}
	}

	assert(materialRegistry.cpuInfo.size() == materialRegistry.gpuInfo.size());
	assert(materialRegistry.gpuInfo.size() == materialRegistry.materialColors.size());

	// Lock GPUInfo mutex and populate GPUInfo for the current material ID.
	{
		UNIQUE_LOCK(MaterialRegistryGPUInfo, materialRegistry.gpuInfoMutex);
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
	}

	UNIQUE_LOCK(MaterialRegistryMap, materialRegistry.mapMutex);
	materialRegistry.map[name] = MaterialInstance(currentIndex);
	return materialRegistry.map[name];
}

bool MaterialManager::IsMaterialValid(const MaterialID materialID) const
{
	if (materialID == MATERIAL_INVALID)
		return false;

	// Shared mutex lock for CPUinfo and check if the materialID exceeds the vector.
	{
		SHARED_LOCK(MaterialRegistryCPUInfo, materialRegistry.cpuInfoMutex);
		if (materialID >= materialRegistry.cpuInfo.size())
			return false;
	}

	return true;
}

bool MaterialManager::IsMaterialValid(const MaterialInstance& materialInstance) const
{
	return IsMaterialValid(materialInstance.materialID);
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
	if (!IsMaterialValid(materialID))
		return;

	LOG_INFO("Releaseing material %i", materialID);

	materialRegistry.refCounts[materialID].store(0U);

	// Mutex lock for cpu, gpu info and material color.
	{ 
		SCOPED_UNIQUE_LOCK(
			materialRegistry.cpuInfoMutex, 
			materialRegistry.gpuInfoMutex, 
			materialRegistry.materialColorsMutex);
			
		materialRegistry.cpuInfo[materialID] = {}; // Reset to default
		materialRegistry.gpuInfo[materialID] = {};
		materialRegistry.materialColors[materialID] = {};
	}

	// Mutex for material instance map.
	{ 
		UNIQUE_LOCK(MaterialRegistryMap, materialRegistry.mapMutex);
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
	}

	UNIQUE_LOCK(releasedMaterialIDs, releasedMaterialIDsMutex);
	releasedMaterialIDs.emplace_back(materialID);
}

std::optional<MaterialInstance> MaterialManager::GetMaterialInstance(const std::wstring& name)
{
	SHARED_LOCK(MaterialRegistryMap, materialRegistry.mapMutex);

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

UINT MaterialManager::GetCPUFlags(const MaterialInstance& materialInstance) const
{
	return materialRegistry.cpuInfo[materialInstance.materialID].flags;
}

UINT MaterialManager::GetGPUFlags(const MaterialInstance& materialInstance) const
{
	return materialRegistry.gpuInfo[materialInstance.materialID].flags;
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