#pragma once
#include <Texture.h>
#include <TypesCompat.h>

#define MATERIAL_INVALID UINT_MAX

using MaterialID = std::uint32_t;
using MaterialColorID = std::uint32_t;

struct MaterialInstance
{
	friend class MaterialManager;
	friend class DeferredRenderer;
	friend class MeshManager;

	MaterialInstance() = default;

	MaterialInstance(MaterialID instanceID) { materialID = instanceID; };

	MaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs);

	bool GetMaterialInstance(const std::wstring& name);

	static MaterialID CreateMaterial(const std::wstring& name, const MaterialColor& material);

	void SetMaterial(MaterialID materialId);
	
	void SetFlags(UINT flags);

	void AddFlag(UINT flag);

	UINT GetCPUFlags();
	UINT GetGPUFlags();

	MaterialID GetMaterialInstanceID() const 
	{
		return materialID;
	}

private:
	static MaterialID CreateMaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs);
	MaterialID materialID{UINT_MAX};
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