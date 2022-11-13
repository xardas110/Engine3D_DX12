#pragma once
#include <Texture.h>

using MaterialID = std::uint32_t;
using MaterialInstanceID = UINT;

struct MaterialInstance
{
	friend class MaterialManager;
	friend class DeferredRenderer;

private:
	MaterialID materialID{};
};

