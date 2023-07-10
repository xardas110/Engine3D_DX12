#pragma once
#include <Common/TypesCompat.h>

struct DirectionalLight
{
public:
	DirectionalLight();

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