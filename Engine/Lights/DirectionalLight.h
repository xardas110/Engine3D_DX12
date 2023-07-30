#pragma once
#include <Common/TypesCompat.h>

struct DirectionalLight
{
public:
	DirectionalLight();

	void SetDirection(const DirectX::XMFLOAT3 newDir);
	void SetColor(const DirectX::XMFLOAT3 newColor);
	void SetItensity(float itensity);

	void UpdateUI();

	const DirectionalLightCB& GetData() const;

private:
	DirectionalLightCB data;
};