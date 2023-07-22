#pragma once
#include <TypesCompat.h>
#include <Material.h>
#include <map>
#include <array>
#include <type_traits>
#include <assert.h>
#include <AssetManagerDefines.h>

class MaterialManager;

// The MaterialManagerAccess class serves as an exclusive access point for creating the MaterialManagerAccess class. 
class MaterialManagerAccess
{
    friend class AssetManager;
private:
    static std::unique_ptr<MaterialManager> CreateMaterialManager();
};

// Responsible for managing materials, follows the same pattern as texture manager.
class MaterialManager
{
    // Granting access to certain classes that might need to interact directly
    // with the internals of the MaterialManager.
    friend class MaterialManagerAccess;
    friend struct MaterialInstance;
    friend class MaterialManagerTest;

    MaterialManager() = default;

public:
    MaterialManager(const MaterialManager&) = delete;
    MaterialManager& operator=(const MaterialManager&) = delete;

    MaterialManager(MaterialManager&&) = delete;
    MaterialManager& operator=(MaterialManager&&) = delete;

    // Loads a material with the specified name, textureIDs, and materialColor.
    // If ThreadSafe is enabled, it returns a copy; otherwise, it returns a reference.
    template<bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto LoadMaterial(const std::wstring& name, const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor)
        -> std::conditional_t<ThreadSafe, MaterialInstance, const MaterialInstance&>;

    // Retrieves a material instance based on its name.
    std::optional<MaterialInstance> GetMaterialInstance(const std::wstring& name);

    // Set flags for a specific material instance.
    void SetFlags(const MaterialInstance& materialInstance, const UINT flags);

    // Adds flags to a material instance.
    void AddFlags(const MaterialInstance& materialInstance, const UINT flags);

    // Checks if a material instance is valid.
    bool IsMaterialValid(const MaterialInstance& materialInstance) const;

    // Retrieves a specific texture instance associated with a material.
    TextureInstance GetTextureInstance(const MaterialInstance& materialInstance, MaterialType::Type type);

    // Retrieves the name of the material.
    // If ThreadSafe is enabled, it returns a copy; otherwise, it returns a reference.
    template <bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto GetMaterialName(const MaterialInstance& materialInstance) const
        ->std::conditional_t<ThreadSafe, std::wstring, const std::wstring&>;

    // Retrieves the color of the material.
    // If ThreadSafe is enabled, it returns a copy; otherwise, it returns a reference.
    template <bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto GetMaterialColor(const MaterialInstance& materialInstance)
        -> std::conditional_t<ThreadSafe, MaterialColor, const MaterialColor&>;

    // Retrieves the CPU flags for the material.
    UINT GetCPUFlags(const MaterialInstance& materialInstance) const;

    // Retrieves the GPU flags for the material.
    UINT GetGPUFlags(const MaterialInstance& materialInstance) const;

    // Retrieves the CPU information for the material.
    // If ThreadSafe is enabled, it returns a copy; otherwise, it returns a reference.
    template <bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto GetMaterialCPUInfoData() const;

    // Retrieves the GPU information for the material.
    // If ThreadSafe is enabled, it returns a copy; otherwise, it returns a reference.
    template <bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto GetMaterialGPUInfoData() const;

    // Retrieves the color data of all materials.
    // If ThreadSafe is enabled, it returns a copy; otherwise, it returns a reference.
    template <bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto GetMaterialColorData() const;

private:
    // Checks the validity of a material based on its ID.
    bool IsMaterialValid(const MaterialID materialID) const;

    // Creates a new material. If ThreadSafe is enabled, it returns a copy; otherwise, it returns a reference.
    template<bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto CreateMaterial(const std::wstring& name, const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor)
        -> std::conditional_t<ThreadSafe, MaterialInstance, const MaterialInstance&>;

    // Increases the reference count for a given material ID.
    void IncreaseRefCount(const MaterialID materialID);

    // Decreases the reference count for a given material ID.
    void DecreaseRefCount(const MaterialID materialID);

    // Releases a material by its ID.
    void ReleaseMaterial(const MaterialID materialID);

    // Registry to contain all the materialinfo for CPU and GPU.
    struct MaterialRegistry
    {
        std::vector<MaterialInfoCPU> cpuInfo; // CPU information for materials
        std::vector<MaterialInfoGPU> gpuInfo; // GPU information for materials
        std::vector<MaterialColor> materialColors; // Colors associated with materials
        std::array<std::atomic<RefCount>, MATERIAL_MANAGER_MAX_MATERIALS> refCounts; // Reference counts for materials
        std::map<std::wstring, MaterialInstance> map; // Mapping of material names to instances

        // Mutexes to ensure thread safety during concurrent access/modifications.
        CREATE_MUTEX(map);
        CREATE_MUTEX(materialColors);
        CREATE_MUTEX(gpuInfo);
        CREATE_MUTEX(cpuInfo);
    } materialRegistry;

    // Mutex for releasedMaterialIDs to ensure thread safety.
    CREATE_MUTEX(releasedMaterialIDs);

    // Contains IDs of materials that have been released.
    std::vector<MaterialID> releasedMaterialIDs;

    // Constants for default material values.
    const static std::wstring ms_NoMaterial;
    const static MaterialColor ms_DefaultMaterialColor;
};

// Function to populate GPU Information.
inline void PopulateGPUInfo(MaterialInfoGPU& gpuInfo, const TextureInstance& instance, UINT& gpuField)
{
    if (instance.IsValid())
    {
        gpuField = instance.GetTextureGPUHandle().value();
    }
}

// Function template to load material.
// Ensures that materials are only created once and reused if requested again.
template<bool ThreadSafe>
inline auto MaterialManager::LoadMaterial(
    const std::wstring& name,
    const MaterialInfoCPU& textureIDs,
    const MaterialColor& materialColor)
    -> std::conditional_t<ThreadSafe, MaterialInstance, const MaterialInstance&>
{
    // Check for thread safety requirements during compile-time.
    if constexpr (ThreadSafe)
        static_assert(ASSET_MANAGER_THREAD_SAFE &&
            "This function is not thread safe since ASSET_MANAGER_THREAD_SAFE == false");

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

// Function template to create material.
template<bool ThreadSafe>
inline auto MaterialManager::CreateMaterial(
    const std::wstring& name,
    const MaterialInfoCPU& textureIDs,
    const MaterialColor& materialColor)
    -> std::conditional_t<ThreadSafe, MaterialInstance, const MaterialInstance&>
{
    // Check for thread safety requirements during compile-time.
    if constexpr (ThreadSafe)
        static_assert(ASSET_MANAGER_THREAD_SAFE &&
            "This function is not thread safe since ASSET_MANAGER_THREAD_SAFE == false");

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
    return materialRegistry.map[name];
}

// Retrieves the name associated with a material instance.
template <bool ThreadSafe>
auto MaterialManager::GetMaterialName(const MaterialInstance& materialInstance) const
-> std::conditional_t<ThreadSafe, std::wstring, const std::wstring&>
{
    // Check for thread safety requirements during compile-time.
    if constexpr (ThreadSafe)
        static_assert(ASSET_MANAGER_THREAD_SAFE &&
            "The function is not thread safe if ASSET_MANAGER_THREAD_SAFE == false");

    // Return default name if material is invalid.
    if (!IsMaterialValid(materialInstance.materialID))
        return MaterialManager::ms_NoMaterial;

    SHARED_LOCK(MaterialRegistryMap, materialRegistry.mapMutex);
    for (const auto& [name, id] : materialRegistry.map)
    {
        if (id.materialID == materialInstance.materialID)
            return name;
    }

    return MaterialManager::ms_NoMaterial;
}

// Retrieves the color associated with a material instance.
template <bool ThreadSafe>
auto MaterialManager::GetMaterialColor(const MaterialInstance& materialInstance)
-> std::conditional_t<ThreadSafe, MaterialColor, const MaterialColor&>
{
    // Check for thread safety requirements during compile-time.
    if constexpr (ThreadSafe)
        static_assert(ASSET_MANAGER_THREAD_SAFE &&
            "The function is not thread safe if ASSET_MANAGER_THREAD_SAFE == false");

    // Return default color if material is invalid.
    if (!IsMaterialValid(materialInstance.materialID))
        return MaterialManager::ms_DefaultMaterialColor;

    SHARED_LOCK(MaterialRegistryMaterialColors, materialRegistry.materialColorsMutex);
    return materialRegistry.materialColors[materialInstance.materialID];
}

// Below are specialized functions to retrieve material information 
// based on whether thread safety is required or not.

template <>
inline auto MaterialManager::GetMaterialCPUInfoData<true>() const
{
    static_assert(ASSET_MANAGER_THREAD_SAFE &&
        "The function is not thread safe if ASSET_MANAGER_THREAD_SAFE == false");
    SHARED_LOCK(MaterialRegistryCPUInfo, materialRegistry.cpuInfoMutex);
    return materialRegistry.cpuInfo;
}

template <>
inline auto MaterialManager::GetMaterialCPUInfoData<false>() const
{
    return std::ref(materialRegistry.cpuInfo);
}

template <>
inline auto MaterialManager::GetMaterialGPUInfoData<true>() const
{
    static_assert(ASSET_MANAGER_THREAD_SAFE &&
        "The function is not thread safe if ASSET_MANAGER_THREAD_SAFE == false");
    SHARED_LOCK(MaterialRegistryGPUInfo, materialRegistry.gpuInfoMutex);
    return materialRegistry.gpuInfo;
}

template <>
inline auto MaterialManager::GetMaterialGPUInfoData<false>() const
{
    return std::ref(materialRegistry.gpuInfo);
}

template <>
inline auto MaterialManager::GetMaterialColorData<true>() const
{
    static_assert(ASSET_MANAGER_THREAD_SAFE &&
        "The function is not thread safe if ASSET_MANAGER_THREAD_SAFE == false");
    SHARED_LOCK(MaterialRegistryMaterialColors, materialRegistry.materialColorsMutex);
    return materialRegistry.materialColors;
}

template <>
inline auto MaterialManager::GetMaterialColorData<false>() const
{
    return std::ref(materialRegistry.materialColors);
}