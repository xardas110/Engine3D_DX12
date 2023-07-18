#pragma once
#include <TypesCompat.h>
#include <Material.h>
#include <map>

class TextureData;
class TextureManager;

class MaterialManager
{
    friend class AssetManager;
    friend class DeferredRenderer;
    friend class Editor;
    friend class MaterialInstance;

public:  // Public member functions
    // Function prototypes are single lined for readability
    std::optional<MaterialInfoID> CreateMaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs);
    bool GetMaterialInstance(const std::wstring& name, MaterialInstance& outMaterialInstance);
    MaterialInfoID CreateMaterial(const std::wstring& name, const MaterialColor& material);
    MaterialInfoID GetMaterial(const std::wstring& name);
    void SetMaterial(MaterialInfoID materialId, MaterialInfoID matInstanceID);
    void SetFlags(MaterialInfoID materialID, const UINT flags);
    void AddFlags(MaterialInfoID materialID, const UINT flags);
    TextureInstance GetTextureInstance(MaterialType::Type type, MaterialInfoID matInstanceId);
    const std::wstring& GetMaterialInstanceName(MaterialInfoID matInstanceId) const;
    const std::wstring& GetMaterialName(MaterialInfoID matInstanceId) const;
    MaterialColor& GetUserDefinedMaterial(MaterialInfoID matInstanceId);

private:  // Private member functions
    MaterialManager(const TextureManager& textureData);

private:  // Private structures
    // MaterialData and InstanceData are separated for readability
    struct MaterialColorRegistry
    {
        std::vector<MaterialColor> materials;
        std::map<std::wstring, MaterialColorID> map;
    } materialColorRegistry;

    struct MaterialInfoRegistry
    {
        // 1:1 relations. GPU info will be batched to GPU
        std::vector<MaterialInfoCPU> cpuInfo;
        std::vector<MaterialInfoGPU> gpuInfo;
        std::map<std::wstring, MaterialInfoID> map;
    } materialInfoRegistry;

private:  // Private member variables
    const TextureManager& m_TextureManager;
};