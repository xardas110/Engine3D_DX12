#pragma once
#include <TypesCompat.h>
#include <Material.h>
#include <map>
#include <array>

#define MATERIAL_MANAGER_MAX_MATERIALS 10000

class TextureData;
class TextureManager;

class MaterialManager
{
    friend class AssetManager;
    friend class Editor;
    friend class MaterialInstance;

public:  
    //Creates a material if it does not exist in the cache, or gets the current material in the cache.
    const MaterialInstance& LoadMaterial(const std::wstring& name, const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor);
        
    //Gets a material if it exists
    std::optional<MaterialInstance> GetMaterialInstance(const std::wstring& name);

    void SetFlags(const MaterialInstance& materialInstance, const UINT flags);
    void AddFlags(const MaterialInstance& materialInstance, const UINT flags);

    bool IsMaterialValid(const MaterialInstance& materialInstance) const;

    TextureInstance GetTextureInstance(const MaterialInstance& materialInstance, MaterialType::Type type);

    const std::wstring& GetMaterialName(const MaterialInstance& materialInstance) const;
    const MaterialColor& GetMaterialColor(const MaterialInstance& materialInstance);

    const std::vector<MaterialInfoCPU>& GetMaterialCPUInfoData() const;
    const std::vector<MaterialInfoGPU>& GetMaterialGPUInfoData() const;
    const std::vector<MaterialColor>&   GetMaterialColorData() const;

private:  
    const MaterialInstance& CreateMaterial(const std::wstring& name, const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor);

    void IncreaseRefCount(const MaterialID materialID);
    void DecreaseRefCount(const MaterialID materialID);
    void ReleaseMaterial(const MaterialID materialID);

    struct MaterialRegistry
    {
        // 1:1 relations. GPU info will be batched to GPU
        std::vector<MaterialInfoCPU> cpuInfo;
        std::vector<MaterialInfoGPU> gpuInfo;
        std::vector<MaterialColor> materialColors;

        std::array<std::atomic<RefCount>, MATERIAL_MANAGER_MAX_MATERIALS> refCounts;

        std::map<std::wstring, MaterialInstance> map;
    } materialRegistry;

    std::vector<MaterialID> releasedMaterialIDs;
};