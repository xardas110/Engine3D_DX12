#pragma once
#include <TypesCompat.h>
#include <Material.h>
#include <map>
#include <array>
#include <type_traits>
#include <assert.h>
#include <AssetManagerDefines.h>
#include <event.hpp>

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
    std::optional<MaterialInstance> LoadMaterial(const std::wstring& name, const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor);

    // Retrieves a material instance based on its name.
    std::optional<MaterialInstance> GetMaterialInstance(const std::wstring& name);

    // Set flags for a specific material instance.
    void SetFlags(const MaterialInstance& materialInstance, const UINT flags);

    // Adds flags to a material instance.
    void AddFlags(const MaterialInstance& materialInstance, const UINT flags);

    // Checks if a material instance is valid.
    bool IsMaterialValid(const MaterialInstance& materialInstance) const;

    // Checks if material color or opacity texture has opacity.
    bool HasOpacity(const MaterialInstance materialInstnace) const;

    // Retrieves a specific texture instance associated with a material.
    std::optional<TextureInstance> GetTextureInstance(const MaterialInstance& materialInstance, MaterialType::Type type) const;

    // Retrieves the name of the material.
    // If ThreadSafe is enabled, it returns a copy; otherwise, it returns a reference.
    template <bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto GetMaterialName(const MaterialInstance& materialInstance) const
        ->std::conditional_t<ThreadSafe, std::wstring, const std::wstring&>;

    // Retrieves the color of the material.
    // If ThreadSafe is enabled, it returns a copy; otherwise, it returns a reference.
    template <bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto GetMaterialColor(const MaterialInstance& materialInstance) const
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

    /**
   * Attach a member function to the "Material Created" event.
   *
   * This function allows external components to be notified when
   * a new material instance is created. Whenever the event triggers,
   * the attached member function of the specified class will be called.
   *
   * Template Parameters:
   *   TClass: The class type of the member function.
   *   TRet: The return type of the member function.
   *   Args: The argument types for the member function.
   *
   * @param[in] func Pointer to the member function to be attached.
   * @param[in] obj Pointer to the object whose member function will be called.
   */
    template<typename TClass, typename TRet, typename ...Args>
    void AttachToMaterialCreatedEvent(TRet(TClass::* func) (Args...), TClass* obj)
    {
        materialInstanceCreatedEvent.attach(func, *obj);
    }

    /**
     * Attach a member function to the "Material Deleted" event.
     *
     * This function allows external components to be notified when
     * a material instance is deleted. Whenever the event triggers,
     * the attached member function of the specified class will be called.
     *
     * Template Parameters:
     *   TClass: The class type of the member function.
     *   TRet: The return type of the member function.
     *   Args: The argument types for the member function.
     *
     * @param[in] func Pointer to the member function to be attached.
     * @param[in] obj Pointer to the object whose member function will be called.
     */
    template<typename TClass, typename TRet, typename ...Args>
    void AttachToMaterialDeletedEvent(TRet(TClass::* func) (Args...), TClass* obj)
    {
        materialInstanceDeletedEvent.attach(func, *obj);
    }

private:
    // Checks the validity of a material based on its ID.
    bool IsMaterialValid(const MaterialID materialID) const;

    std::optional<MaterialInstance> CreateMaterial(const std::wstring& name, const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor);

    // Increases the reference count for a given material ID.
    void IncreaseRefCount(const MaterialID materialID);

    // Decreases the reference count for a given material ID.
    void DecreaseRefCount(const MaterialID materialID);

    // Releases a material by its ID.
    void ReleaseMaterial(const MaterialID materialID);

    // Registry to contain all the materialinfo for CPU and GPU.
    struct MaterialRegistry
    {
        MaterialRegistry() = default;

        MaterialRegistry(const MaterialRegistry&) = delete;
        MaterialRegistry& operator=(const MaterialRegistry&) = delete;

        MaterialRegistry(MaterialRegistry&&) = delete;
        MaterialRegistry& operator=(MaterialRegistry&&) = delete;

        // 1:1 relations
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

    // Events
    event::event<void(const MaterialInstance&)> materialInstanceCreatedEvent;
    event::event<void(const MaterialID&)> materialInstanceDeletedEvent;

    // Mutex for releasedMaterialIDs to ensure thread safety.
    CREATE_MUTEX(releasedMaterialIDs);

    // Contains IDs of materials that have been released.
    std::vector<MaterialID> releasedMaterialIDs;

    // Constants for default material values.
    const static std::wstring ms_NoMaterial;
    const static MaterialColor ms_DefaultMaterialColor;
};

// Retrieves the name associated with a material instance.
template <bool ThreadSafe>
auto MaterialManager::GetMaterialName(const MaterialInstance& materialInstance) const
-> std::conditional_t<ThreadSafe, std::wstring, const std::wstring&>
{
    // Check for thread safety requirements during compile-time.
    if constexpr (ThreadSafe)
        static_assert(ASSET_MANAGER_THREAD_SAFE &&
            "This function is not thread safe since ASSET_MANAGER_THREAD_SAFE == false");

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
auto MaterialManager::GetMaterialColor(const MaterialInstance& materialInstance) const
-> std::conditional_t<ThreadSafe, MaterialColor, const MaterialColor&>
{
    // Check for thread safety requirements during compile-time.
    if constexpr (ThreadSafe)
        static_assert(ASSET_MANAGER_THREAD_SAFE &&
            "This function is not thread safe since ASSET_MANAGER_THREAD_SAFE == false");

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
    assert(ASSET_MANAGER_THREAD_SAFE &&
        "This function is not thread safe since ASSET_MANAGER_THREAD_SAFE == false");
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
    assert(ASSET_MANAGER_THREAD_SAFE &&
        "This function is not thread safe since ASSET_MANAGER_THREAD_SAFE == false");
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
    assert(ASSET_MANAGER_THREAD_SAFE &&
        "This function is not thread safe since ASSET_MANAGER_THREAD_SAFE == false");
    SHARED_LOCK(MaterialRegistryMaterialColors, materialRegistry.materialColorsMutex);
    return materialRegistry.materialColors;
}

template <>
inline auto MaterialManager::GetMaterialColorData<false>() const
{
    return std::ref(materialRegistry.materialColors);
}