#pragma once
#include <Common/TypesCompat.h>

struct DirectionalLight
{
	DirectionalLight()
	{
		SetDirection(XMFLOAT3(-0.064f, -0.231f, 0.971f));
		SetColor(XMFLOAT3(1.f, 1.f, 1.f));
		SetItensity(20.f);
	}

	void SetDirection(const XMFLOAT3 newDir);
	void SetColor(const XMFLOAT3 newColor);
	void SetItensity(float itensity);

	DirectionalLightCB data;
};