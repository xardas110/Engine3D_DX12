#pragma once
#include <TypesCompat.h>
#include <Material.h>
#include <map>

class TextureData;
class TextureManager;

class MaterialManager
{
    friend class AssetManager;
    friend class Editor;
    friend class MaterialInstance;

public:  // Public member functions
    // Function prototypes are single lined for readability
    std::optional<MaterialID> CreateMaterialInstance(const std::wstring& name, 
        const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor);

    bool GetMaterialInstance(const std::wstring& name, MaterialInstance& outMaterialInstance);
    MaterialID GetMaterial(const std::wstring& name);

    void SetFlags(MaterialID materialID, const UINT flags);
    void AddFlags(MaterialID materialID, const UINT flags);

    TextureInstance GetTextureInstance(MaterialType::Type type, MaterialID matInstanceId);
    const std::wstring& GetMaterialInstanceName(MaterialID matInstanceId) const;
    const MaterialColor& GetMaterialColor(const MaterialID materialID);

    const std::vector<MaterialInfoCPU>& GetMaterialCPUInfoData() const;
    const std::vector<MaterialInfoGPU>& GetMaterialGPUInfoData() const;
    const std::vector<MaterialColor>&   GetMaterialColorData() const;

private:  
    struct MaterialRegistry
    {
        // 1:1 relations. GPU info will be batched to GPU
        std::vector<MaterialInfoCPU> cpuInfo;
        std::vector<MaterialInfoGPU> gpuInfo;
        std::vector<MaterialColor> materialColors;
        std::map<std::wstring, MaterialID> map;
    } materialRegistry;
};