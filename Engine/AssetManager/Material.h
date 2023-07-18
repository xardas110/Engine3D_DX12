#pragma once
#include <Texture.h>
#include <TypesCompat.h>

using MaterialID = std::uint32_t;
using MaterialInstanceID = UINT;

struct MaterialInstance
{
	friend class MaterialManager;
	friend class DeferredRenderer;
	friend class MeshManager;

	MaterialInstance() = default;

	MaterialInstance(UINT instanceID) { materialInstanceID = instanceID; };

	MaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs)
	{
		materialInstanceID = CreateMaterialInstance(name, textureIDs);
	}

	MaterialInstanceID CreateMaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs);
	bool GetMaterialInstance(const std::wstring& name);

	static MaterialID CreateMaterial(const std::wstring& name, const MaterialColor& material);

	void SetMaterial(MaterialID materialId);
	
	void SetFlags(UINT flags);

	void AddFlag(UINT flag);

	UINT GetCPUFlags();
	UINT GetGPUFlags();

	MaterialInstanceID GetMaterialInstanceID() const 
	{
		return materialInstanceID;
	}

private:
	MaterialInstanceID materialInstanceID{UINT_MAX};
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