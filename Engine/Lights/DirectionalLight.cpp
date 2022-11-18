#include <EnginePCH.h>
#include <DirectionalLight.h>

void DirectionalLight::SetDirection(const XMFLOAT3 newDir)
{
	data.direction = XMLoadFloat3(&newDir);

	data.direction = XMVector3Normalize(data.direction);
}

void DirectionalLight::SetColor(const XMFLOAT3 newColor)
{
	data.color = XMLoadFloat3(&newColor);
}

void DirectionalLight::SetItensity(float itensity)
{
	data.color.m128_f32[3] = itensity;
}