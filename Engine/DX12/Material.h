#pragma once
#include <Texture.h>
#include <TypesCompat.h>

using MaterialID = std::uint32_t;
using MaterialInstanceID = UINT;

struct MaterialInstance
{
	friend class MaterialManager;
	friend class DeferredRenderer;

	MaterialInstanceID CreateMaterialInstance(const std::wstring& name, const MaterialInfo& textureIDs);
	bool GetMaterialInstance(const std::wstring& name);

	MaterialInstanceID GetMaterialInstanceID() const 
	{
		return materialID;
	}

private:
	MaterialInstanceID materialID{UINT_MAX};
};

