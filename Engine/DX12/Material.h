#pragma once
#include <Texture.h>
#include <RayTracingHlslCompat.h>

using MaterialID = std::uint32_t;
using MaterialInstanceID = UINT;

struct MaterialInstance
{
	friend class MaterialManager;
	friend class DeferredRenderer;

	bool CreateMaterialInstance(const std::wstring& name, const MaterialInfo& textureIDs);
	bool GetMaterialInstance(const std::wstring& name);

private:
	MaterialID materialID{UINT_MAX};
};

