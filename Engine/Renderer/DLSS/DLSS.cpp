#include <EnginePCH.h>
#include <DLSS/DLSS.h>
#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>

DLSS::DLSS()
{
	NVSDK_NGX_Result Result = NVSDK_NGX_Result_Fail;

    ID3D12Device* d3ddevice = Application::Get().GetDevice().Get();

    Result = NVSDK_NGX_D3D12_Init(APP_ID, L"Assets", d3ddevice);
    
    if (NVSDK_NGX_FAILED(Result)) throw std::exception("Failed to Init NGX dx12");

    Result = NVSDK_NGX_D3D12_GetCapabilityParameters(&m_ngxParameters);

    if (NVSDK_NGX_FAILED(Result))
    {
        ShutDown();

        std::wstring resultMsg = std::wstring(GetNGXResultAsString(Result));

        std::string errorMsg = "NVSDK_NGX_GetCapabilityParameters failed, code = 0x%08x, info: " + std::string(resultMsg.begin(), resultMsg.end());

        throw std::exception(errorMsg.c_str());
    }

    int dlssAvailable = 0;
    NVSDK_NGX_Result ResultDLSS = m_ngxParameters->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &dlssAvailable);
    if (ResultDLSS != NVSDK_NGX_Result_Success || !dlssAvailable)
    {
        // More details about what failed (per feature init result)
        NVSDK_NGX_Result FeatureInitResult = NVSDK_NGX_Result_Fail;
        NVSDK_NGX_Parameter_GetI(m_ngxParameters, NVSDK_NGX_Parameter_SuperSampling_FeatureInitResult, (int*)&FeatureInitResult);
        printf("NVIDIA DLSS not available on this hardward/platform., FeatureInitResult = 0x%08x, info: %ls", FeatureInitResult, GetNGXResultAsString(FeatureInitResult));
        ShutDown();

        return;
    }

    InitFeatures({3840, 2160}, { 3840, 2160 }, 0, false, 1.f, false, false, NVSDK_NGX_PerfQuality_Value::NVSDK_NGX_PerfQuality_Value_Balanced);
}

DLSS::~DLSS()
{
    ShutDown();
}


void DLSS::InitFeatures(DirectX::XMINT2 optimalRenderSize, DirectX::XMINT2 outDisplaySize, int isContentHDR, bool depthInverted, float depthScale, bool enableSharpening, bool enableAutoExposure, NVSDK_NGX_PerfQuality_Value qualValue)
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
        return;
    }

    auto fenceVal = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceVal);
}

void DLSS::ShutDown()
{
    NVSDK_NGX_D3D12_DestroyParameters(m_ngxParameters);
    NVSDK_NGX_D3D12_Shutdown();
}