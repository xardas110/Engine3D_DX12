#include <EnginePCH.h>
#include <HDR/HDR.h>
#include <imgui.h>
#include <Helpers.h>
#include <Application.h>
#include <DX12Helper.h>

#include <HDR_VS.h>
#include <HDR_PS.h>

#include <Exposure_CS.h>
#include <Histogram_CS.h>

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

void HDR::UpdateEyeAdaptionGUI()
{
    ImGui::SliderFloat("eyeAdaptionSpeedDown", &eyeAdaption.eyeAdaptionSpeedDown, 0.f, 10.0f);
    ImGui::SliderFloat("eyeAdaptionSpeedUp", &eyeAdaption.eyeAdaptionSpeedUp, 0.f, 10.0f);
    ImGui::SliderFloat("lowPercentile", &eyeAdaption.lowPercentile, 0.f, 1.0f);
    ImGui::SliderFloat("highPercentile", &eyeAdaption.highPercentile, 0.f, 1.0f);

    ImGui::SliderFloat("minAdaptedLuminance", &eyeAdaption.minAdaptedLuminance, 0.f, 20.0f);
    ImGui::SliderFloat("maxAdaptedLuminance", &eyeAdaption.maxAdaptedLuminance, 0.f, 20.0f);

    ImGui::SliderFloat("whitePoint", &eyeAdaption.whitePoint, 0.f, 10.0f);

    ImGui::SliderFloat("exposureBias", &eyeAdaption.exposureBias, -10.0f, 10.0f);
}

void HDR::UpdateGUI()
{
    ImGui::Begin("Tonemapping");
    {
        static bool bEye = (bool)tonemapParameters.bEyeAdaption;
        ImGui::Checkbox("EyeAdaption", &bEye);
        tonemapParameters.bEyeAdaption = (int)bEye; //for shader

        if (bEye)
        { 
            UpdateEyeAdaptionGUI();
            ImGui::End();
            return;           
        }

        ImGui::TextWrapped("Use the Exposure slider to adjust the overall exposure of the HDR scene.");
        ImGui::SliderFloat("Exposure", &tonemapParameters.Exposure, -10.0f, 10.0f);
        ImGui::SameLine(); ShowHelpMarker("Adjust the overall exposure of the HDR scene.");
        ImGui::SliderFloat("Gamma", &tonemapParameters.Gamma, 0.01f, 5.0f);
        ImGui::SameLine(); ShowHelpMarker("Adjust the Gamma of the output image.");

        ImGui::SliderFloat("MinLogLuminance", &tonemapParameters.minLogLuminance, -30.f, 30.f);
        ImGui::SliderFloat("MaxLogLuminamce", &tonemapParameters.maxLogLuminamce, -30.f, 30.f);

        const char* toneMappingMethods[] = {
            "Linear",
            "Reinhard",
            "Reinhard Squared",
            "ACES Filmic",
            "Uncharted"
        };

        ImGui::Combo("Tonemapping Methods", (int*)(&tonemapParameters.TonemapMethod), toneMappingMethods, sizeof(toneMappingMethods) / sizeof(toneMappingMethods[0]));

        switch (tonemapParameters.TonemapMethod)
        {
        case TM_Linear:
            ImGui::SliderFloat("Max Brightness", &tonemapParameters.MaxLuminance, 1.0f, 10.0f);
            ImGui::SameLine(); ShowHelpMarker("Linearly scale the HDR image by the maximum brightness.");
            break;
        case TM_Reinhard:
            ImGui::SliderFloat("Reinhard Constant", &tonemapParameters.K, 0.01f, 10.0f);
            ImGui::SameLine(); ShowHelpMarker("The Reinhard constant is used in the denominator.");
            break;
        case TM_ReinhardSq:
            ImGui::SliderFloat("Reinhard Constant", &tonemapParameters.K, 0.01f, 10.0f);
            ImGui::SameLine(); ShowHelpMarker("The Reinhard constant is used in the denominator.");
            break;
        case TM_ACESFilmic:
            ImGui::SliderFloat("Shoulder Strength", &tonemapParameters.A, 0.01f, 5.0f);
            ImGui::SliderFloat("Linear Strength", &tonemapParameters.B, 0.0f, 100.0f);
            ImGui::SliderFloat("Linear Angle", &tonemapParameters.C, 0.0f, 1.0f);
            ImGui::SliderFloat("Toe Strength", &tonemapParameters.D, 0.01f, 1.0f);
            ImGui::SliderFloat("Toe Numerator", &tonemapParameters.E, 0.0f, 10.0f);
            ImGui::SliderFloat("Toe Denominator", &tonemapParameters.F, 1.0f, 10.0f);
            ImGui::SliderFloat("Linear White", &tonemapParameters.LinearWhite, 1.0f, 120.0f);
            break;
        default:
            break;
        }
    }

    if (ImGui::Button("Reset to Defaults"))
    {
        tonemapParameters = TonemapCB();
    }

    ImGui::End();
}

TonemapCB& HDR::GetTonemapCB()
{
    return tonemapParameters;
}


HDR::HDR(int nativeWidth, int nativeHeight)
{
    CreateRenderTarget(nativeWidth, nativeHeight);
    CreatePipeline();
    CreateExposurePSO();
    CreateHistogramPSO();
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
    
    D3D12_RESOURCE_ALLOCATION_INFO histoInfo;
    histoInfo.Alignment = 0;
    histoInfo.SizeInBytes = sizeof(UINT) * HISTOGRAM_BINS;

    auto histogramDesc = CD3DX12_RESOURCE_DESC::Buffer(histoInfo, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    histogram = Texture(histogramDesc, nullptr, TextureUsage::RenderTarget, L"HistogramBuffer");

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

    CD3DX12_DESCRIPTOR_RANGE1 exposureHeap = {};
    exposureHeap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    exposureHeap.NumDescriptors = 1;
    exposureHeap.BaseShaderRegister = 0;
    exposureHeap.RegisterSpace = 0;
    exposureHeap.OffsetInDescriptorsFromTableStart = 0;
    exposureHeap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    CD3DX12_ROOT_PARAMETER1 rootParameters[HDRParam::Size];
    rootParameters[HDRParam::ColorTexture].InitAsDescriptorTable(1, &textureHeap);
    rootParameters[HDRParam::ExposureTex].InitAsDescriptorTable(1, &exposureHeap);
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
    exposureHeap.BaseShaderRegister = 1;
    exposureHeap.RegisterSpace = 0;
    exposureHeap.OffsetInDescriptorsFromTableStart = 0;
    exposureHeap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;

    CD3DX12_DESCRIPTOR_RANGE1 histogramHeap = {};
    histogramHeap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    histogramHeap.NumDescriptors = 1;
    histogramHeap.BaseShaderRegister = 0;
    histogramHeap.RegisterSpace = 0;
    histogramHeap.OffsetInDescriptorsFromTableStart = 0;
    //histogramHeap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;

    CD3DX12_ROOT_PARAMETER1 rootParameters[ExposureParam::Size];
    rootParameters[ExposureParam::ExposureTex].InitAsDescriptorTable(1, &exposureHeap);
    rootParameters[ExposureParam::Histogram].InitAsDescriptorTable(1, &histogramHeap);
    rootParameters[ExposureParam::TonemapCB].InitAsConstantBufferView(2,0);

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

void HDR::CreateHistogramPSO()
{
    auto device = Application::Get().GetDevice();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE1 histogramHeap = {};
    histogramHeap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    histogramHeap.NumDescriptors = 1;
    histogramHeap.BaseShaderRegister = 0;
    histogramHeap.RegisterSpace = 0;
    histogramHeap.OffsetInDescriptorsFromTableStart = 0;
    histogramHeap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;

    CD3DX12_DESCRIPTOR_RANGE1 sourceHeap = {};
    sourceHeap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    sourceHeap.NumDescriptors = 1;
    sourceHeap.BaseShaderRegister = 0;
    sourceHeap.RegisterSpace = 0;
    sourceHeap.OffsetInDescriptorsFromTableStart = 0;
    //sourceHeap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;

    CD3DX12_ROOT_PARAMETER1 rootParameters[HistogramParam::Size];
    rootParameters[HistogramParam::Histogram].InitAsDescriptorTable(1, &histogramHeap);
    rootParameters[HistogramParam::SourceTex].InitAsDescriptorTable(1, &sourceHeap, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[HistogramParam::TonemapCB].InitAsConstantBufferView(2, 0);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Init_1_1(HistogramParam::Size,
        rootParameters, 0, nullptr);

    histogramRT.SetRootSignatureDesc(
        rootSignatureDesc.Desc_1_1,
        featureData.HighestVersion
    );

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_CS CS;
    } pipelineStateStream;

    pipelineStateStream.pRootSignature = histogramRT.GetRootSignature().Get();
    pipelineStateStream.CS = { g_Histogram_CS, sizeof(g_Histogram_CS) };

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&histogramPSO)));
}

void HDR::CreateUAVViews()
{
    auto device = Application::Get().GetDevice();
   
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
    D3D12_BUFFER_UAV bufferUAV;
    bufferUAV.FirstElement = 0;
    bufferUAV.NumElements = HISTOGRAM_BINS;
    bufferUAV.StructureByteStride = 0;
    bufferUAV.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    bufferUAV.CounterOffsetInBytes = 0;

    desc.Buffer = bufferUAV;
    desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    desc.Format = DXGI_FORMAT_R32_TYPELESS;

    device->CreateUnorderedAccessView(exposureTex.GetD3D12Resource().Get(), nullptr, &uavDesc, exposureHeap.SetHandle(0));
    device->CreateUnorderedAccessView(histogram.GetD3D12Resource().Get(), nullptr, &desc, histogramHeap.SetHandle(0));
    device->CreateUnorderedAccessView(histogram.GetD3D12Resource().Get(), nullptr, &desc, histogramClearHeap.SetHandle(0));
}

void HDR::OnResize(int nativeWidth, int nativeHeight)
{
    renderTarget.Resize(nativeWidth, nativeHeight);
}