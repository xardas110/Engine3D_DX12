#pragma once
#include <Common/TypesCompat.h>

struct DirectionalLight
{
	DirectionalLight()
	{
		SetDirection(XMFLOAT3(-0.815f, -0.573f, 0.086f));
		SetColor(XMFLOAT3(1.f, 0.5f, 1.f));
		SetItensity(0.7f);
	}

	void SetDirection(const XMFLOAT3 newDir);
	void SetColor(const XMFLOAT3 newColor);
	void SetItensity(float itensity);

	void UpdateUI();

	const DirectionalLightCB& GetData()
	{
		data.tanAngularRadius = tan(DirectX::XMConvertToRadians(data.angularDiameter * 0.5f));
		return data;
	}

private:
	DirectionalLightCB data;
};