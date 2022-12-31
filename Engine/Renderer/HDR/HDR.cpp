#include <EnginePCH.h>
#include <HDR/HDR.h>
#include <imgui.h>
#include <Helpers.h>
#include <Application.h>
#include <DX12Helper.h>

#include <HDR_VS.h>
#include <HDR_PS.h>

#include <Exposure_CS.h>

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
            "ACES Filmic",
            "Uncharted"
        };

        ImGui::Combo("Tonemapping Methods", (int*)(&g_TonemapParameters.TonemapMethod), toneMappingMethods, sizeof(toneMappingMethods) / sizeof(toneMappingMethods[0]));

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


HDR::HDR(int nativeWidth, int nativeHeight)
{
    CreateRenderTarget(nativeWidth, nativeHeight);
    CreatePipeline();
    CreateExposurePSO();
    CreateUAVViews();
}

void HDR::CreateRenderTarget(int nativeWidth, int nativeHeight)
{
    auto outputTextureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, nativeWidth, nativeHeight);
    outputTextureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    outputTextureDesc.MipLevels = 1;

    auto exposureTexDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT, 1, 1);
    exposureTexDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    exposureTexDesc.MipLevels = 1;

    exposureTex = Texture(exposureTexDesc, nullptr,
        TextureUsage::RenderTarget,
        L"Exposure texture");

    Texture outputTexture = Texture(outputTextureDesc, &ClearValue(outputTextureDesc.Format, { 0.f, 0.f, 0.f, 0.f }),
        TextureUsage::RenderTarget,
        L"HDR output texture");

    renderTarget.AttachTexture(AttachmentPoint::Color0, outputTexture);
}

void HDR::CreatePipeline()
{
    auto device = Application::Get().GetDevice();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE1 textureHeap = {};
    textureHeap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    textureHeap.NumDescriptors = 1;
    textureHeap.BaseShaderRegister = 0;
    textureHeap.RegisterSpace = 0;
    textureHeap.OffsetInDescriptorsFromTableStart = 0;
    textureHeap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    CD3DX12_ROOT_PARAMETER1 rootParameters[HDRParam::Size];
    rootParameters[HDRParam::ColorTexture].InitAsDescriptorTable(1, &textureHeap);
    rootParameters[HDRParam::TonemapCB].InitAsConstantBufferView(0, 0);

    CD3DX12_STATIC_SAMPLER_DESC samplers[2];
    samplers[0] = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT);
    samplers[1] = CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_ANISOTROPIC);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Init_1_1(HDRParam::Size,
        rootParameters, 2, samplers);

    rootSignature.SetRootSignatureDesc(
        rootSignatureDesc.Desc_1_1,
        featureData.HighestVersion
    );

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    } pipelineStateStream;

    CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    pipelineStateStream.Rasterizer = rasterizerDesc;

    pipelineStateStream.pRootSignature = rootSignature.GetRootSignature().Get();

    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    pipelineStateStream.VS = { g_HDR_VS, sizeof(g_HDR_VS) };
    pipelineStateStream.PS = { g_HDR_PS, sizeof(g_HDR_PS) };

    pipelineStateStream.RTVFormats = renderTarget.GetRenderTargetFormats();

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipeline)));
}

void HDR::CreateExposurePSO()
{
    auto device = Application::Get().GetDevice();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE1 exposureHeap = {};
    exposureHeap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    exposureHeap.NumDescriptors = 1;
    exposureHeap.BaseShaderRegister = 0;
    exposureHeap.RegisterSpace = 0;
    exposureHeap.OffsetInDescriptorsFromTableStart = 0;
    exposureHeap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;

    CD3DX12_ROOT_PARAMETER1 rootParameters[ExposureParam::Size];
    rootParameters[ExposureParam::ExposureTex].InitAsDescriptorTable(1, &exposureHeap);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Init_1_1(ExposureParam::Size,
        rootParameters, 0, nullptr);

    exposureRT.SetRootSignatureDesc(
        rootSignatureDesc.Desc_1_1,
        featureData.HighestVersion
    );

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_CS CS;
    } pipelineStateStream;


    pipelineStateStream.pRootSignature = exposureRT.GetRootSignature().Get();
    pipelineStateStream.CS = { g_Exposure_CS, sizeof(g_Exposure_CS) };

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&exposurePSO)));
}

D3D12_GPU_DESCRIPTOR_HANDLE HDR::CreateUAVViews()
{
    auto device = Application::Get().GetDevice();
   
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    
    device->CreateUnorderedAccessView(exposureTex.GetD3D12Resource().Get(), nullptr, &uavDesc, heap.SetHandle(0));

    return heap.GetHandleAtStart();
}

void HDR::OnResize(int nativeWidth, int nativeHeight)
{
    renderTarget.Resize(nativeWidth, nativeHeight);
}