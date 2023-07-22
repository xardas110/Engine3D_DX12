#pragma once
#include <TypesCompat.h>
#include <Material.h>
#include <map>
#include <array>
#include <type_traits>
#include <assert.h>
#include <AssetManagerDefines.h>

class TextureData;
class TextureManager;

class MaterialManager
{
    friend class AssetManager;
    friend class Editor;
    friend class MaterialInstance;

public:

    template<bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto LoadMaterial(const std::wstring& name, const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor)
        -> std::conditional_t<ThreadSafe, MaterialInstance, const MaterialInstance&>;

    std::optional<MaterialInstance> GetMaterialInstance(const std::wstring& name);
    void SetFlags(const MaterialInstance& materialInstance, const UINT flags);
    void AddFlags(const MaterialInstance& materialInstance, const UINT flags);
    bool IsMaterialValid(const MaterialInstance& materialInstance) const;

    TextureInstance GetTextureInstance(const MaterialInstance& materialInstance, MaterialType::Type type);

    template <bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto GetMaterialName(const MaterialInstance& materialInstance) const
        ->std::conditional_t<ThreadSafe, std::wstring, const std::wstring&>;

    template <bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto GetMaterialColor(const MaterialInstance& materialInstance)
        -> std::conditional_t<ThreadSafe, MaterialColor, const MaterialColor&>;

    UINT GetCPUFlags(const MaterialInstance& materialInstance) const;
    UINT GetGPUFlags(const MaterialInstance& materialInstance) const;

    template <bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto GetMaterialCPUInfoData() const;

    template <bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto GetMaterialGPUInfoData() const;

    template <bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto GetMaterialColorData() const;

private:
    bool IsMaterialValid(const MaterialID materialID) const;

    template<bool ThreadSafe = ASSET_MANAGER_THREAD_SAFE>
    auto CreateMaterial(const std::wstring& name, const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor)
        -> std::conditional_t<ThreadSafe, MaterialInstance, const MaterialInstance&>;

    void IncreaseRefCount(const MaterialID materialID);
    void DecreaseRefCount(const MaterialID materialID);
    void ReleaseMaterial(const MaterialID materialID);

    struct MaterialRegistry
    {
        std::vector<MaterialInfoCPU> cpuInfo;
        std::vector<MaterialInfoGPU> gpuInfo;
        std::vector<MaterialColor> materialColors;
        std::array<std::atomic<RefCount>, MATERIAL_MANAGER_MAX_MATERIALS> refCounts;
        std::map<std::wstring, MaterialInstance> map;

        CREATE_MUTEX(map);
        CREATE_MUTEX(materialColors);
        CREATE_MUTEX(gpuInfo);
        CREATE_MUTEX(cpuInfo);
    } materialRegistry;

    CREATE_MUTEX(releasedMaterialIDs);
    std::vector<MaterialID> releasedMaterialIDs;

    const static std::wstring ms_NoMaterial;
    const static MaterialColor ms_DefaultMaterialColor;
};

inline void PopulateGPUInfo(MaterialInfoGPU& gpuInfo, const TextureInstance& instance, UINT& gpuField)
{
    if (instance.IsValid())
    {
        gpuField = instance.GetTextureGPUHandle().value();
    }
}

template<bool ThreadSafe>
inline auto MaterialManager::LoadMaterial(
    const std::wstring& name, 
    const MaterialInfoCPU& textureIDs, 
    const MaterialColor& materialColor)
-> std::conditional_t<ThreadSafe, MaterialInstance, const MaterialInstance&>
{
    if constexpr (ThreadSafe)
        static_assert(ASSET_MANAGER_THREAD_SAFE && 
            "This function is not thread safe since ASSET_MANAGER_THREAD_SAFE == false");

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

template<bool ThreadSafe>
inline auto MaterialManager::CreateMaterial(
    const std::wstring& name, 
    const MaterialInfoCPU& textureIDs, 
    const MaterialColor& materialColor)
-> std::conditional_t<ThreadSafe, MaterialInstance, const MaterialInstance&>
{

    if constexpr (ThreadSafe)
        static_assert(ASSET_MANAGER_THREAD_SAFE &&
            "This function is not thread safe since ASSET_MANAGER_THREAD_SAFE == false");

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

template <bool ThreadSafe>
auto MaterialManager::GetMaterialName(const MaterialInstance& materialInstance) const
-> std::conditional_t<ThreadSafe, std::wstring, const std::wstring&>
{
    if constexpr (ThreadSafe)
        static_assert(ASSET_MANAGER_THREAD_SAFE && 
            "The function is not thread safe if ASSET_MANAGER_THREAD_SAFE == false");

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

template <bool ThreadSafe>
auto MaterialManager::GetMaterialColor(const MaterialInstance& materialInstance)
-> std::conditional_t<ThreadSafe, MaterialColor, const MaterialColor&>
{
    if constexpr (ThreadSafe)
        static_assert(ASSET_MANAGER_THREAD_SAFE && 
            "The function is not thread safe if ASSET_MANAGER_THREAD_SAFE == false");

    if (!IsMaterialValid(materialInstance.materialID))
        return MaterialManager::ms_DefaultMaterialColor;

    SHARED_LOCK(MaterialRegistryMaterialColors, materialRegistry.materialColorsMutex);
    return materialRegistry.materialColors[materialInstance.materialID];
}

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
    static_assert(ASSET_MANAGER_THREAD_SAFE && "The function is not thread safe if ASSET_MANAGER_THREAD_SAFE == false");
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
    static_assert(ASSET_MANAGER_THREAD_SAFE && "The function is not thread safe if ASSET_MANAGER_THREAD_SAFE == false");
    SHARED_LOCK(MaterialRegistryMaterialColors, materialRegistry.materialColorsMutex);
    return materialRegistry.materialColors;
}

template <>
inline auto MaterialManager::GetMaterialColorData<false>() const
{
    return std::ref(materialRegistry.materialColors);
}