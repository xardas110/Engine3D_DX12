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
    std::optional<MaterialID> CreateMaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs);
    bool GetMaterialInstance(const std::wstring& name, MaterialInstance& outMaterialInstance);
    MaterialID CreateMaterial(const std::wstring& name, const MaterialColor& material);
    MaterialID GetMaterial(const std::wstring& name);
    void SetMaterial(MaterialID materialId, MaterialID matInstanceID);
    void SetFlags(MaterialID materialID, const UINT flags);
    void AddFlags(MaterialID materialID, const UINT flags);
    TextureInstance GetTextureInstance(MaterialType::Type type, MaterialID matInstanceId);
    const std::wstring& GetMaterialInstanceName(MaterialID matInstanceId) const;
    const std::wstring& GetMaterialName(MaterialID matInstanceId) const;
    MaterialColor& GetUserDefinedMaterial(MaterialID matInstanceId);

private:  // Private member functions
    MaterialManager(const TextureManager& textureData);

private:  // Private structures
    // MaterialData and InstanceData are separated for readability
    struct MaterialColorRegistry
    {
        std::vector<MaterialColor> materials;
        std::map<std::wstring, MaterialID> map;
    } materialColorRegistry;

    struct InstanceData
    {
        // 1:1 relations. GPU info will be batched to GPU
        std::vector<MaterialInfoCPU> cpuInfo;
        std::vector<MaterialInfoGPU> gpuInfo;
        std::map<std::wstring, MaterialID> map;
    } instanceData;

private:  // Private member variables
    const TextureManager& m_TextureManager;
};