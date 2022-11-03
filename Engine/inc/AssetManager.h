#pragma once
#include "Mesh.h"

namespace Primitives
{
	enum
	{
		Cube,
		Size
	};
}

class AssetManager
{
	friend class Application;
	friend class DeferredRenderer;
	friend class std::default_delete<AssetManager>;

	AssetManager();
	~AssetManager();

	static std::unique_ptr<AssetManager> CreateInstance();

private:
	Mesh m_Primitives[Primitives::Size];
};