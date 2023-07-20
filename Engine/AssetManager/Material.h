#pragma once
#include <Texture.h>
#include <TypesCompat.h>

#define MATERIAL_INVALID UINT_MAX

using MaterialID = std::uint32_t;
using RefCount = std::uint32_t;

struct MaterialInfoCPU;

class MaterialInstance
{
    friend class MaterialManager;

public:
    MaterialInstance() = default;
    MaterialInstance(MaterialID instanceID) : materialID(instanceID) {}
    MaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor);

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
    MaterialID materialID{ MATERIAL_INVALID };
};

struct MaterialInfoCPU
{
    friend class MaterialInfoHelper;
    friend class MaterialHelper;

    MaterialInfoCPU();

    const TextureInstance& GetTexture(MaterialType::Type type) const
    {
        if (type >= 0 && type < MaterialType::NumMaterialTypes)
            return textures[type];
        else
            throw std::out_of_range("Invalid MaterialType");
    }

    UINT pad MATERIAL_ID_NULL;
    UINT flags DEFAULT_NULL;

private:
    TextureInstance textures[MaterialType::NumMaterialTypes];
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
