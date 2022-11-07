#pragma once
#include <Texture.h>

namespace MaterialType
{
	enum Type
	{
		Ambient,
		Albedo,
		Diffuse = Albedo,
		Specular,
		Height,
		Normal = Height,
		Roughness,
		Metallic,
		Opacity,
		Emissive,
		Size
	};
}

class Material
{
	friend class AssetManager;
	friend class DeferredRenderer;

public:
	bool HasMaterial(MaterialType::Type type) const
	{
		return textures[type].IsValid();
	}
private:
	Texture textures[MaterialType::Size];
};
