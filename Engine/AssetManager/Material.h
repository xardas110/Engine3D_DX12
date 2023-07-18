#pragma once
#include <Texture.h>
#include <TypesCompat.h>

#define MATERIAL_INVALID UINT_MAX

using MaterialID = std::uint32_t;
using MaterialColorID = std::uint32_t;

class MaterialInstance
{
    friend class MaterialManager;

public:
    MaterialInstance() = default;
    MaterialInstance(MaterialID instanceID) : materialID(instanceID) {}
    MaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs);

    bool GetMaterialInstance(const std::wstring& name);
    static MaterialID CreateMaterial(const std::wstring& name, const MaterialColor& material);
    void SetMaterial(const MaterialID materialId);
    void SetFlags(const UINT flags);
    void AddFlag(const UINT flag);
    UINT GetCPUFlags() const;
    UINT GetGPUFlags() const;

    MaterialID GetMaterialInstanceID() const
    {
        return materialID;
    }

private:
    MaterialManager* GetMaterialManager() const;
    static MaterialID CreateMaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs);
    MaterialID materialID{ MATERIAL_INVALID };
};

class MaterialInfoHelper
{
public:
	static MaterialInfoCPU PopulateMaterialInfo(const struct AssimpMesh& mesh, int flags);
	static bool IsTextureValid(UINT texture);
};

class MaterialHelper
{
public:
	static MaterialColor CreateMaterial(const struct AssimpMaterialData& materialData);
	static bool IsTransparent(const struct MaterialColor& materialData);
};
