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

	MaterialInstance(const std::wstring& name, const MaterialInfo& textureIDs)
	{
		materialInstanceID = CreateMaterialInstance(name, textureIDs);
	}

	MaterialInstanceID CreateMaterialInstance(const std::wstring& name, const MaterialInfo& textureIDs);
	bool GetMaterialInstance(const std::wstring& name);

	static MaterialID CreateMaterial(const std::wstring& name, const Material& material);

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
