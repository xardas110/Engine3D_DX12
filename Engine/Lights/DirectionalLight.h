#pragma once
#include <Common/TypesCompat.h>

struct DirectionalLight
{
	DirectionalLight()
	{
		SetDirection(XMFLOAT3(-0.26f, -0.938f, -0.245f));
		SetColor(XMFLOAT3(1.f, 1.f, 1.f));
		SetItensity(3.f);
	}

	void SetDirection(const XMFLOAT3 newDir);
	void SetColor(const XMFLOAT3 newColor);
	void SetItensity(float itensity);

	DirectionalLightCB data;
};