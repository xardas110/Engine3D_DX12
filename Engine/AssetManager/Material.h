#pragma once
#include <Texture.h>

#define MATERIAL_INVALID UINT_MAX

using MaterialID = std::uint32_t;
using RefCount = std::uint32_t;

struct MaterialInfoCPU;
class MaterialInstance;

//Only specific classes can get the MaterialID due to the ID not being reference counted.
class MaterialInstanceAccess
{
    friend class DeferredRenderer;

    static MaterialID GetMaterialID(const MaterialInstance& materialInstance);
};

class MaterialInstance
{
    friend class MaterialManager;
    friend class MaterialInstanceAccess;
public:
    MaterialInstance();

    explicit MaterialInstance(MaterialID instanceID);

    MaterialInstance(const std::wstring& name,
        const struct MaterialInfoCPU& textureIDs, 
        const struct MaterialColor& materialColor);

    MaterialInstance(const MaterialInstance& other);
    MaterialInstance& operator=(const MaterialInstance& other);

    MaterialInstance(MaterialInstance&& other) noexcept;
    MaterialInstance& operator=(MaterialInstance&& other) noexcept;

    MaterialInstance::~MaterialInstance();

    bool HasOpacity() const;

    void SetFlags(const UINT flags);
    void AddFlag(const UINT flag);
    UINT GetCPUFlags() const;
    UINT GetGPUFlags() const;

private:
    MaterialManager* GetMaterialManager() const;
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
