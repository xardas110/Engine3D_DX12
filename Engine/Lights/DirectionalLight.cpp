#include <EnginePCH.h>
#include <DirectionalLight.h>
#include <imgui.h>

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

void DirectionalLight::UpdateUI()
{
    ImGui::Begin("Directional light settings");
    if (ImGui::SliderFloat3("Direction", &data.direction.m128_f32[0], -1, +1))
    {
        if (XMVector3Equal(data.direction, XMVectorZero()))
        {
            data.direction = { 0.f, -1.f, 0.f, data.direction.m128_f32[3] };
        }
        else
        {
            data.direction = XMVector3Normalize(data.direction);
        }
    }
    ImGui::SliderFloat("Angular Diameter", &data.angularDiameter, 0.f, 10.f);
    ImGui::SliderFloat("Lux", &data.color.m128_f32[3], 0, 20.f);
    ImGui::ColorPicker3("Color", &data.color.m128_f32[0], ImGuiColorEditFlags_Float);
    ImGui::End();
}