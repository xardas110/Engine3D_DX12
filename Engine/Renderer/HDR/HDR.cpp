#include <EnginePCH.h>
#include <HDR/HDR.h>
#include <imgui.h>
#include <Helpers.h>

TonemapCB g_TonemapParameters;

// Number of values to plot in the tonemapping curves.
static const int VALUES_COUNT = 256;
// Maximum HDR value to normalize the plot samples.
static const float HDR_MAX = 12.0f;

// Clamp a value between a min and max range.
template<typename T>
constexpr const T& clamp(const T& val, const T& min = T(0), const T& max = T(1))
{
    return val < min ? min : val > max ? max : val;
}

float LinearTonemapping(float HDR, float max)
{
    if (max > 0.0f)
    {
        return clamp(HDR / max);
    }
    return HDR;
}

float LinearTonemappingPlot(void*, int index)
{
    return LinearTonemapping(index / (float)VALUES_COUNT * HDR_MAX, g_TonemapParameters.MaxLuminance);
}

// Reinhard tone mapping.
// See: http://www.cs.utah.edu/~reinhard/cdrom/tonemap.pdf
float ReinhardTonemapping(float HDR, float k)
{
    return HDR / (HDR + k);
}

float ReinhardTonemappingPlot(void*, int index)
{
    return ReinhardTonemapping(index / (float)VALUES_COUNT * HDR_MAX, g_TonemapParameters.K);
}

float ReinhardSqrTonemappingPlot(void*, int index)
{
    float reinhard = ReinhardTonemapping(index / (float)VALUES_COUNT * HDR_MAX, g_TonemapParameters.K);
    return reinhard * reinhard;
}

// ACES Filmic
// See: https://www.slideshare.net/ozlael/hable-john-uncharted2-hdr-lighting/142
float ACESFilmicTonemapping(float x, float A, float B, float C, float D, float E, float F)
{
    return (((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - (E / F));
}

float ACESFilmicTonemappingPlot(void*, int index)
{
    float HDR = index / (float)VALUES_COUNT * HDR_MAX;
    return ACESFilmicTonemapping(HDR, g_TonemapParameters.A, g_TonemapParameters.B, g_TonemapParameters.C, g_TonemapParameters.D, g_TonemapParameters.E, g_TonemapParameters.F) /
        ACESFilmicTonemapping(g_TonemapParameters.LinearWhite, g_TonemapParameters.A, g_TonemapParameters.B, g_TonemapParameters.C, g_TonemapParameters.D, g_TonemapParameters.E, g_TonemapParameters.F);
}

// Helper to display a little (?) mark which shows a tooltip when hovered.
static void ShowHelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void HDR::UpdateGUI()
{
    ImGui::Begin("Tonemapping");
    {
        ImGui::TextWrapped("Use the Exposure slider to adjust the overall exposure of the HDR scene.");
        ImGui::SliderFloat("Exposure", &g_TonemapParameters.Exposure, -10.0f, 10.0f);
        ImGui::SameLine(); ShowHelpMarker("Adjust the overall exposure of the HDR scene.");
        ImGui::SliderFloat("Gamma", &g_TonemapParameters.Gamma, 0.01f, 5.0f);
        ImGui::SameLine(); ShowHelpMarker("Adjust the Gamma of the output image.");

        const char* toneMappingMethods[] = {
            "Linear",
            "Reinhard",
            "Reinhard Squared",
            "ACES Filmic"
        };

        ImGui::Combo("Tonemapping Methods", (int*)(&g_TonemapParameters.TonemapMethod), toneMappingMethods, 4);

        switch (g_TonemapParameters.TonemapMethod)
        {
        case TM_Linear:
            ImGui::PlotLines("Linear Tonemapping", &LinearTonemappingPlot, nullptr, VALUES_COUNT, 0, nullptr, 0.0f, 1.0f, ImVec2(0, 250));
            ImGui::SliderFloat("Max Brightness", &g_TonemapParameters.MaxLuminance, 1.0f, 10.0f);
            ImGui::SameLine(); ShowHelpMarker("Linearly scale the HDR image by the maximum brightness.");
            break;
        case TM_Reinhard:
            ImGui::PlotLines("Reinhard Tonemapping", &ReinhardTonemappingPlot, nullptr, VALUES_COUNT, 0, nullptr, 0.0f, 1.0f, ImVec2(0, 250));
            ImGui::SliderFloat("Reinhard Constant", &g_TonemapParameters.K, 0.01f, 10.0f);
            ImGui::SameLine(); ShowHelpMarker("The Reinhard constant is used in the denominator.");
            break;
        case TM_ReinhardSq:
            ImGui::PlotLines("Reinhard Squared Tonemapping", &ReinhardSqrTonemappingPlot, nullptr, VALUES_COUNT, 0, nullptr, 0.0f, 1.0f, ImVec2(0, 250));
            ImGui::SliderFloat("Reinhard Constant", &g_TonemapParameters.K, 0.01f, 10.0f);
            ImGui::SameLine(); ShowHelpMarker("The Reinhard constant is used in the denominator.");
            break;
        case TM_ACESFilmic:
            ImGui::PlotLines("ACES Filmic Tonemapping", &ACESFilmicTonemappingPlot, nullptr, VALUES_COUNT, 0, nullptr, 0.0f, 1.0f, ImVec2(0, 250));
            ImGui::SliderFloat("Shoulder Strength", &g_TonemapParameters.A, 0.01f, 5.0f);
            ImGui::SliderFloat("Linear Strength", &g_TonemapParameters.B, 0.0f, 100.0f);
            ImGui::SliderFloat("Linear Angle", &g_TonemapParameters.C, 0.0f, 1.0f);
            ImGui::SliderFloat("Toe Strength", &g_TonemapParameters.D, 0.01f, 1.0f);
            ImGui::SliderFloat("Toe Numerator", &g_TonemapParameters.E, 0.0f, 10.0f);
            ImGui::SliderFloat("Toe Denominator", &g_TonemapParameters.F, 1.0f, 10.0f);
            ImGui::SliderFloat("Linear White", &g_TonemapParameters.LinearWhite, 1.0f, 120.0f);
            break;
        default:
            break;
        }
    }

    if (ImGui::Button("Reset to Defaults"))
    {
        g_TonemapParameters = TonemapCB();
    }

    ImGui::End();
}

TonemapCB& HDR::GetTonemapCB()
{
    return g_TonemapParameters;
}