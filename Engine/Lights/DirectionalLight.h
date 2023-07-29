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

	const DirectionalLightCB& GetData() const;

private:
	DirectionalLightCB data;
};