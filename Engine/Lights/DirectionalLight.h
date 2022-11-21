#pragma once
#include <Common/TypesCompat.h>

struct DirectionalLight
{
	DirectionalLight()
	{
		SetDirection(XMFLOAT3(-0.f, -0.58f, 0.58f));
		SetColor(XMFLOAT3(1.f, 1.f, 1.f));
		SetItensity(1.f);
	}

	void SetDirection(const XMFLOAT3 newDir);
	void SetColor(const XMFLOAT3 newColor);
	void SetItensity(float itensity);

	DirectionalLightCB data;
};