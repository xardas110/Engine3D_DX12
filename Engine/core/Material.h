#pragma once
#include <Texture.h>

using MaterialID = std::uint32_t;

struct MaterialInstance
{
	friend class AssetManager;
	friend class MeshInstance;
	friend class DeferredRenderer;

	MaterialInstance(const std::wstring& name);
private:
	MaterialID materialID{};
};

class Material
{
	friend class AssetManager;
	friend class DeferredRenderer;

private:
	MaterialInfo textureIDs;
};
