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
	friend class std::default_delete<AssetManager>;

	AssetManager();
	~AssetManager();

	static std::unique_ptr<AssetManager> CreateInstance();

private:
	std::unique_ptr<Mesh> m_Primitives[Primitives::Size];
};