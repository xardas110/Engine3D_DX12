#include <EnginePCH.h>
#include <MaterialManager.h>
#include <TextureManager.h>
#include <Texture.h>

const std::wstring MaterialManager::ms_NoMaterial = L"No Material";
const MaterialColor MaterialManager::ms_DefaultMaterialColor = MaterialColor();

std::unique_ptr<MaterialManager> MaterialManagerAccess::CreateMaterialManager()
{
	return std::unique_ptr<MaterialManager>(new MaterialManager);
}

// Function to populate GPU Information.
inline void PopulateGPUInfo(MaterialInfoGPU& gpuInfo, const TextureInstance& instance, UINT& gpuField)
{
    if (instance.IsValid())
    {
        gpuField = instance.GetTextureGPUHandle().value();
    }
}

// Ensures that materials are only created once and reused if requested again.
std::optional<MaterialInstance> MaterialManager::LoadMaterial(
    const std::wstring& name,
    const MaterialInfoCPU& textureIDs,
    const MaterialColor& materialColor)
{
    // Check if material already exists.
    {
        SHARED_LOCK(MaterialRegistryMap, materialRegistry.mapMutex);
        auto found = materialRegistry.map.find(name);
        if (found != materialRegistry.map.end())
        {
            return found->second;
        }
    }

    // If material doesn't exist, create a new one.
    return CreateMaterial(name, textureIDs, materialColor);
}

std::optional<MaterialInstance> MaterialManager::CreateMaterial(
    const std::wstring& name,
    const MaterialInfoCPU& textureIDs,
    const MaterialColor& materialColor)
{
    MaterialID currentIndex = 0;
	
    // Acquire necessary locks and either reuse or create material index.
    {
        SCOPED_UNIQUE_LOCK(
            releasedMaterialIDsMutex,
            materialRegistry.cpuInfoMutex,
            materialRegistry.gpuInfoMutex,
            materialRegistry.materialColorsMutex);

        // If there are released IDs, use them.
        if (!releasedMaterialIDs.empty())
        {
            currentIndex = releasedMaterialIDs.front();
            releasedMaterialIDs.erase(releasedMaterialIDs.begin());
        }
        else
        {
            // Otherwise, get the next index and ensure we haven't exceeded the limit.
            currentIndex = materialRegistry.cpuInfo.size();
            assert(currentIndex < MATERIAL_MANAGER_MAX_MATERIALS && "Material Manager exceeds maximum materials");

            // Add material data.
            materialRegistry.cpuInfo.emplace_back(textureIDs);
            materialRegistry.gpuInfo.emplace_back(MaterialInfoGPU());
            materialRegistry.materialColors.emplace_back(materialColor);
            materialRegistry.refCounts[currentIndex].store(0U);
        }
    }

    // Sanity checks.
    assert(materialRegistry.cpuInfo.size() == materialRegistry.gpuInfo.size());
    assert(materialRegistry.gpuInfo.size() == materialRegistry.materialColors.size());

    // Populate GPU Information.
    {
        UNIQUE_LOCK(MaterialRegistryGPUInfo, materialRegistry.gpuInfoMutex);

        // For every material type, fetch its respective texture and populate its GPU data.
#define POPULATE_GPU_INFO_FOR_TYPE(type) \
            PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.textures[MaterialType::type], materialRegistry.gpuInfo[currentIndex].type)

        POPULATE_GPU_INFO_FOR_TYPE(albedo);
        POPULATE_GPU_INFO_FOR_TYPE(normal);
        POPULATE_GPU_INFO_FOR_TYPE(ao);
        POPULATE_GPU_INFO_FOR_TYPE(emissive);
        POPULATE_GPU_INFO_FOR_TYPE(roughness);
        POPULATE_GPU_INFO_FOR_TYPE(specular);
        POPULATE_GPU_INFO_FOR_TYPE(metallic);
        POPULATE_GPU_INFO_FOR_TYPE(lightmap);
        POPULATE_GPU_INFO_FOR_TYPE(opacity);
        POPULATE_GPU_INFO_FOR_TYPE(height);

#undef POPULATE_GPU_INFO_FOR_TYPE
    }

    // Add the new material instance to the registry map.
    UNIQUE_LOCK(MaterialRegistryMap, materialRegistry.mapMutex);
    materialRegistry.map[name] = MaterialInstance(currentIndex);

    // Send a event that the materialInstance was created.
    materialInstanceCreatedEvent(materialRegistry.map[name]);

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

bool MaterialManager::HasOpacity(const MaterialInstance materialInstnace) const
{
	if (!IsMaterialValid(materialInstnace))
		return false;

	auto materialColor = GetMaterialColor(materialInstnace);

	if (materialColor.diffuse.w < 1.f)
		return true;

	auto opacityTexture = GetTextureInstance(materialInstnace, MaterialType::opacity);

	if (!opacityTexture.has_value())
		return false;

	if (!opacityTexture.value().IsValid())
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

	// Send a event that the materialInstance was deleted.
	materialInstanceDeletedEvent(materialID);
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
	if (!IsMaterialValid(materialInstance.materialID))
		return;

	SCOPED_UNIQUE_LOCK(materialRegistry.cpuInfoMutex, materialRegistry.gpuInfoMutex);
	materialRegistry.gpuInfo[materialInstance.materialID].flags = flags;
	materialRegistry.cpuInfo[materialInstance.materialID].flags = flags;
}

void MaterialManager::AddFlags(const MaterialInstance& materialInstance, const UINT flags)
{
	if (!IsMaterialValid(materialInstance.materialID))
		return;

	SCOPED_UNIQUE_LOCK(materialRegistry.cpuInfoMutex, materialRegistry.gpuInfoMutex);
	materialRegistry.gpuInfo[materialInstance.materialID].flags |= flags;
	materialRegistry.cpuInfo[materialInstance.materialID].flags |= flags;
}

std::optional<TextureInstance> MaterialManager::GetTextureInstance(const MaterialInstance& materialInstance, MaterialType::Type type) const
{
	if (!IsMaterialValid(materialInstance.materialID))
		return std::nullopt;

	SHARED_LOCK(MaterialRegistryCPUInfo, materialRegistry.cpuInfoMutex);
	return materialRegistry.cpuInfo[materialInstance.materialID].textures[type];
}

std::optional<UINT> MaterialManager::GetCPUFlags(const MaterialInstance& materialInstance) const
{
	if (!IsMaterialValid(materialInstance.materialID))
		return std::nullopt;

	SHARED_LOCK(MaterialRegistryCPUInfo, materialRegistry.cpuInfoMutex);
	return materialRegistry.cpuInfo[materialInstance.materialID].flags;
}

std::optional<UINT> MaterialManager::GetGPUFlags(const MaterialInstance& materialInstance) const
{
	if (!IsMaterialValid(materialInstance.materialID))
		return std::nullopt;

	SHARED_LOCK(MaterialRegistryGPUInfo, materialRegistry.gpuInfoMutex);
	return materialRegistry.gpuInfo[materialInstance.materialID].flags;
}

