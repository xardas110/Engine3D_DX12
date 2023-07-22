#include <EnginePCH.h>
#include <MaterialManager.h>
#include <TextureManager.h>
#include <Texture.h>

const std::wstring MaterialManager::ms_NoMaterial = L"No Material";
const MaterialColor MaterialManager::ms_DefaultMaterialColor = MaterialColor();

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

	LOG_INFO("Releasing material: %i", materialID);

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
	SCOPED_UNIQUE_LOCK(materialRegistry.cpuInfoMutex, materialRegistry.gpuInfoMutex);
	materialRegistry.gpuInfo[materialInstance.materialID].flags = flags;
	materialRegistry.cpuInfo[materialInstance.materialID].flags = flags;
}

void MaterialManager::AddFlags(const MaterialInstance& materialInstance, const UINT flags)
{
	SCOPED_UNIQUE_LOCK(materialRegistry.cpuInfoMutex, materialRegistry.gpuInfoMutex);
	materialRegistry.gpuInfo[materialInstance.materialID].flags |= flags;
	materialRegistry.cpuInfo[materialInstance.materialID].flags |= flags;
}

TextureInstance MaterialManager::GetTextureInstance(const MaterialInstance& materialInstance, MaterialType::Type type)
{
	SHARED_LOCK(MaterialRegistryCPUInfo, materialRegistry.cpuInfoMutex);
	return materialRegistry.cpuInfo[materialInstance.materialID].textures[type];
}

UINT MaterialManager::GetCPUFlags(const MaterialInstance& materialInstance) const
{
	SHARED_LOCK(MaterialRegistryCPUInfo, materialRegistry.cpuInfoMutex);
	return materialRegistry.cpuInfo[materialInstance.materialID].flags;
}

UINT MaterialManager::GetGPUFlags(const MaterialInstance& materialInstance) const
{
	SHARED_LOCK(MaterialRegistryGPUInfo, materialRegistry.gpuInfoMutex);
	return materialRegistry.gpuInfo[materialInstance.materialID].flags;
}

