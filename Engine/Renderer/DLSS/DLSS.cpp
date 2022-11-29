#include <EnginePCH.h>
#include <DLSS/DLSS.h>
#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>

#include <nvsdk_ngx.h>
#include <nvsdk_ngx_helpers.h>
#include <d3d12.h>

#include <DX12Helper.h>

const char g_ProjectID[] = "a0f57b54-1daf-4934-90ae-c4035c19df04";

DLSS::DLSS(int width, int height)
{
    InitTexture(width, height);
    InitializeNGX(width, height);
}

DLSS::~DLSS()
{
}

void DLSS::InitTexture(int width, int height)
{
    auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
    colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    colorDesc.MipLevels = 1;

    resolvedTexture = Texture(colorDesc, nullptr,
        TextureUsage::RenderTarget,
        L"DLSS resolved Color");
}

void DLSS::InitializeNGX(int width, int height)
{
    NVSDK_NGX_Result Result = NVSDK_NGX_Result_Fail;

    m_Device = Application::Get().GetDevice();

    Result = NVSDK_NGX_D3D12_Init(APP_ID, L"", m_Device.Get());

    if (NVSDK_NGX_FAILED(Result)) throw std::exception("Failed to Init NGX dx12");

    Result = NVSDK_NGX_D3D12_GetCapabilityParameters(&m_ngxParameters);

    if (NVSDK_NGX_FAILED(Result))
    {
        ShutDown();

        std::wstring resultMsg = std::wstring(GetNGXResultAsString(Result));

        std::string errorMsg = "NVSDK_NGX_GetCapabilityParameters failed, code = 0x%08x, info: " + std::string(resultMsg.begin(), resultMsg.end());

        throw std::exception(errorMsg.c_str());
    }

    // If NGX Successfully initialized then it should set those flags in return
    int needsUpdatedDriver = 0;
    unsigned int minDriverVersionMajor = 0;
    unsigned int minDriverVersionMinor = 0;
    unsigned int featureInitResult = 0;

    NVSDK_NGX_Result ResultUpdatedDriver = m_ngxParameters->Get(NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &needsUpdatedDriver);
    NVSDK_NGX_Result ResultMinDriverVersionMajor = m_ngxParameters->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMajor, &minDriverVersionMajor);
    NVSDK_NGX_Result ResultMinDriverVersionMinor = m_ngxParameters->Get(NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMinor, &minDriverVersionMinor);

    if (ResultUpdatedDriver == NVSDK_NGX_Result_Success &&
        ResultMinDriverVersionMajor == NVSDK_NGX_Result_Success &&
        ResultMinDriverVersionMinor == NVSDK_NGX_Result_Success)
    {
        if (needsUpdatedDriver)
        {
            printf("NVIDIA DLSS cannot be loaded due to outdated driver. Minimum Driver Version required : %u.%u\n", minDriverVersionMajor, minDriverVersionMinor);

            ShutDown();

            throw std::exception();
        }
        else
        {
            printf("NVIDIA DLSS Minimum driver version was reported as : %u.%u", minDriverVersionMajor, minDriverVersionMinor);
            std::cout << minDriverVersionMajor << " " << minDriverVersionMinor << std::endl;
        }
    }
    else
    {
        printf("NVIDIA DLSS Minimum driver version was not reported.\n");
    }

    int dlssAvailable = 0;
    NVSDK_NGX_Result ResultDLSS = m_ngxParameters->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &dlssAvailable);
    if (ResultDLSS != NVSDK_NGX_Result_Success || !dlssAvailable)
    {
        // More details about what failed (per feature init result)
        NVSDK_NGX_Result FeatureInitResult = NVSDK_NGX_Result_Fail;
        NVSDK_NGX_Parameter_GetI(m_ngxParameters, NVSDK_NGX_Parameter_SuperSampling_FeatureInitResult, (int*)&FeatureInitResult);
        printf("NVIDIA DLSS not available on this hardware/platform., FeatureInitResult = 0x%08x, info: %ls", FeatureInitResult, GetNGXResultAsString(FeatureInitResult));
        ShutDown();

        throw std::exception("NVIDIA DLSS not available on this hardware/platform, see console for more information");
    }

    InitWithRecommendedSettings(width, height);
}

void DLSS::QueryOptimalSettings(DirectX::XMUINT2 inDisplaySize, NVSDK_NGX_PerfQuality_Value inQualValue, DlssRecommendedSettings* outRecommendedSettings)
{
    outRecommendedSettings->m_ngxRecommendedOptimalRenderSize = inDisplaySize;
    outRecommendedSettings->m_ngxDynamicMaximumRenderSize = inDisplaySize;
    outRecommendedSettings->m_ngxDynamicMinimumRenderSize = inDisplaySize;
    outRecommendedSettings->m_ngxRecommendedSharpness = 0.0f;

    NVSDK_NGX_Result Result = NGX_DLSS_GET_OPTIMAL_SETTINGS(m_ngxParameters,
        inDisplaySize.x, inDisplaySize.y, inQualValue,
        &outRecommendedSettings->m_ngxRecommendedOptimalRenderSize.x, &outRecommendedSettings->m_ngxRecommendedOptimalRenderSize.y,
        &outRecommendedSettings->m_ngxDynamicMaximumRenderSize.x, &outRecommendedSettings->m_ngxDynamicMaximumRenderSize.y,
        &outRecommendedSettings->m_ngxDynamicMinimumRenderSize.x, &outRecommendedSettings->m_ngxDynamicMinimumRenderSize.y,
        &outRecommendedSettings->m_ngxRecommendedSharpness);

    if (NVSDK_NGX_FAILED(Result))
    {
        outRecommendedSettings->m_ngxRecommendedOptimalRenderSize = inDisplaySize;
        outRecommendedSettings->m_ngxDynamicMaximumRenderSize = inDisplaySize;
        outRecommendedSettings->m_ngxDynamicMinimumRenderSize = inDisplaySize;
        outRecommendedSettings->m_ngxRecommendedSharpness = 0.0f;

        printf("Querying Optimal Settings failed! code = 0x%08x, info: %ls", Result, GetNGXResultAsString(Result));

        throw std::exception("Querying Optimal Settings failed!");
    }
}

void DLSS::InitFeatures(DirectX::XMUINT2 optimalRenderSize, DirectX::XMUINT2 outDisplaySize, int isContentHDR, bool depthInverted, float depthScale, bool enableSharpening, bool enableAutoExposure, NVSDK_NGX_PerfQuality_Value qualValue)
{
    auto commandQueue = Application::Get().GetCommandQueue();
    auto commandList = commandQueue->GetCommandList();

    unsigned int CreationNodeMask = 1;
    unsigned int VisibilityNodeMask = 1;
    NVSDK_NGX_Result ResultDLSS = NVSDK_NGX_Result_Fail;

    int MotionVectorResolutionLow = 1; // we let the Snippet do the upsampling of the motion vector
    // Next create features	
    int DlssCreateFeatureFlags = NVSDK_NGX_DLSS_Feature_Flags_None;
    DlssCreateFeatureFlags |= MotionVectorResolutionLow ? NVSDK_NGX_DLSS_Feature_Flags_MVLowRes : 0;
    DlssCreateFeatureFlags |= isContentHDR ? NVSDK_NGX_DLSS_Feature_Flags_IsHDR : 0;
    DlssCreateFeatureFlags |= depthInverted ? NVSDK_NGX_DLSS_Feature_Flags_DepthInverted : 0;
    DlssCreateFeatureFlags |= enableSharpening ? NVSDK_NGX_DLSS_Feature_Flags_DoSharpening : 0;
    DlssCreateFeatureFlags |= enableAutoExposure ? NVSDK_NGX_DLSS_Feature_Flags_AutoExposure : 0;

    NVSDK_NGX_DLSS_Create_Params DlssCreateParams;

    memset(&DlssCreateParams, 0, sizeof(DlssCreateParams));

    DlssCreateParams.Feature.InWidth = optimalRenderSize.x;
    DlssCreateParams.Feature.InHeight = optimalRenderSize.y;
    DlssCreateParams.Feature.InTargetWidth = outDisplaySize.x;
    DlssCreateParams.Feature.InTargetHeight = outDisplaySize.y;
    DlssCreateParams.Feature.InPerfQualityValue = qualValue;
    DlssCreateParams.InFeatureCreateFlags = DlssCreateFeatureFlags;

    ResultDLSS = NGX_D3D12_CREATE_DLSS_EXT(commandList->GetGraphicsCommandList().Get(), CreationNodeMask, VisibilityNodeMask, &m_dlssFeature, m_ngxParameters, &DlssCreateParams);

    if (NVSDK_NGX_FAILED(ResultDLSS))
    {
        printf("Failed to create DLSS Features = 0x%08x, info: %ls", ResultDLSS, GetNGXResultAsString(ResultDLSS));
        throw std::exception("Failed to create DLSS Features, see console for more information");
    }

    auto fenceVal = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceVal);
}

void DLSS::InitWithRecommendedSettings(int width, int height)
{
    QueryOptimalSettings(
        { (unsigned)width, (unsigned)height },
        NVSDK_NGX_PerfQuality_Value::NVSDK_NGX_PerfQuality_Value_Balanced,
        &recommendedSettings);

    InitFeatures(recommendedSettings.m_ngxRecommendedOptimalRenderSize, {(unsigned)width, (unsigned)height}, false, true, 1.f, false, false, NVSDK_NGX_PerfQuality_Value::NVSDK_NGX_PerfQuality_Value_Balanced);
}

void DLSS::EvaluateSuperSampling(
    CommandList* commandList,
    const DlssTextures& texs,
    int width, int height,
    bool bResetAccumulation,
    float flSharpness,
    bool bUseNgxSdkExtApi)
{
    NVSDK_NGX_Result Result;

    NVSDK_NGX_D3D12_DLSS_Eval_Params D3D12DlssEvalParams;

    memset(&D3D12DlssEvalParams, 0, sizeof(D3D12DlssEvalParams));

    D3D12DlssEvalParams.Feature.pInColor = texs.unresolvedColor->GetD3D12Resource().Get();
    D3D12DlssEvalParams.Feature.pInOutput = texs.resolvedColor->GetD3D12Resource().Get();
    D3D12DlssEvalParams.pInDepth = texs.depth->GetD3D12Resource().Get();
    D3D12DlssEvalParams.pInMotionVectors = texs.motionVectors->GetD3D12Resource().Get();
    D3D12DlssEvalParams.pInExposureTexture = nullptr;

    D3D12DlssEvalParams.InJitterOffsetX = 0.f;
    D3D12DlssEvalParams.InJitterOffsetY = 0.f;

    D3D12DlssEvalParams.Feature.InSharpness = flSharpness;

    D3D12DlssEvalParams.InReset = bResetAccumulation;

    D3D12DlssEvalParams.InMVScaleX = 1.f;
    D3D12DlssEvalParams.InMVScaleY = 1.f;

    D3D12DlssEvalParams.InColorSubrectBase = { 0U,0U };
    D3D12DlssEvalParams.InDepthSubrectBase = { 0U,0U };
    D3D12DlssEvalParams.InTranslucencySubrectBase = { 0U,0U };
    D3D12DlssEvalParams.InMVSubrectBase = { 0U,0U };
 
    D3D12DlssEvalParams.InRenderSubrectDimensions = { 
        (unsigned)recommendedSettings.m_ngxRecommendedOptimalRenderSize.x, (unsigned)recommendedSettings.m_ngxRecommendedOptimalRenderSize.y };

    Result = NGX_D3D12_EVALUATE_DLSS_EXT(commandList->GetGraphicsCommandList().Get(), m_dlssFeature, m_ngxParameters, &D3D12DlssEvalParams);

    if (NVSDK_NGX_FAILED(Result))
    {
        printf("Failed to NVSDK_NGX_D3D12_EvaluateFeature for DLSS, code = 0x%08x, info: %ls\n", Result, GetNGXResultAsString(Result));
    }
}

DirectX::XMUINT2 DLSS::OnResize(int width, int height)
{
    InitWithRecommendedSettings(width, height);

    resolvedTexture.Resize(width, height);

    return recommendedSettings.m_ngxRecommendedOptimalRenderSize;
}

void DLSS::ReleaseDLSSFeatures()
{
    NVSDK_NGX_Result ResultDLSS = (m_dlssFeature != nullptr) ? NVSDK_NGX_D3D12_ReleaseFeature(m_dlssFeature) : NVSDK_NGX_Result_Success;
    if (NVSDK_NGX_FAILED(ResultDLSS))
    {
        printf("Failed to NVSDK_NGX_D3D12_ReleaseFeature, code = 0x%08x, info: %ls", ResultDLSS, GetNGXResultAsString(ResultDLSS));
    }
    m_dlssFeature = nullptr;
}

void DLSS::ShutDown()
{
    ReleaseDLSSFeatures();
  
    auto result = NVSDK_NGX_D3D12_DestroyParameters(m_ngxParameters);

    if (NVSDK_NGX_FAILED(result)) throw std::exception();

    NVSDK_NGX_D3D12_Shutdown();

    if (NVSDK_NGX_FAILED(result)) throw std::exception();
}