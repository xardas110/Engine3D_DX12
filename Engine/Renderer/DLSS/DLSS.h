#pragma once

#include <nvsdk_ngx.h>
#include <CommandList.h>
#include <Texture.h>
#include <eventcpp/event.hpp>
#include <StaticDescriptorHeap.h>
#include <Camera.h>

#define APP_ID 231313132

/*
// maximum HDR value in the scene
#define NGX_HDRMax 1000.0f

// when flickering is detected bigger value will use more history to suppress it
#define NGX_FlickerFactor 0.5f

// threshold to determine low vs high contrast. low contrast areas get more current frame than history
#define NGX_ContrastFactor 0.05f

// higher value includes more of the current frame in motion (which can result in sharper images but also flickering)
#define NGX_MotionFactor 0.1f

// bigger the value more current frame will be used vs history (sharper vs blurrier image)
#define NGX_HistoryBlendFactor 0.15f

// higher the value more sharpening applies (and more halo around high contrast areas)
#define NGX_SharpenThreshold 0.3f

// bigger the value more history gets rejected when motion delta is high (current vs previous frame) to avoid ghosting
#define NGX_RejectFactor 0.02f
*/

struct DlssRecommendedSettings
{
	float     m_ngxRecommendedSharpness = 0.01f;
	DirectX::XMUINT2 m_ngxRecommendedOptimalRenderSize = { ~(0u), ~(0u) };
	DirectX::XMUINT2 m_ngxDynamicMaximumRenderSize = { ~(0u), ~(0u) };
	DirectX::XMUINT2 m_ngxDynamicMinimumRenderSize = { ~(0u), ~(0u) };
};

struct DlssTextures
{
	//required textures
	DlssTextures(const Texture* unresolvedColor, const Texture* resolvedColor, const Texture* motionVectors, const Texture* depth, const Texture* exposure)
		: unresolvedColor(unresolvedColor), resolvedColor(resolvedColor) , motionVectors(motionVectors) , depth(depth), exposure(exposure) {}

	const Texture* unresolvedColor{ nullptr };
	const Texture* resolvedColor{ nullptr };
	const Texture* motionVectors{ nullptr };
	const Texture* motionVectors3D{ nullptr };
	const Texture* depth{ nullptr };
	const Texture* exposure{ nullptr };
	const Texture* linearDepth{ nullptr };
};

struct DLSS
{
private:
	friend class DeferredRenderer;
	friend class Editor;
	friend struct std::default_delete<DLSS>;

	DLSS(int width, int height);
	~DLSS();

	//Returns handle to heap start
	D3D12_GPU_DESCRIPTOR_HANDLE CreateSRVViews();

	void InitializeNGX(int width, int height);

	void InitTexture(int width, int height);

	void InitFeatures(
		DirectX::XMUINT2 optimalRenderSize, 
		DirectX::XMUINT2 outDisplaySize, 
		int isContentHDR, bool depthInverted, 
		float depthScale, bool enableSharpening, 
		bool enableAutoExposure, NVSDK_NGX_PerfQuality_Value qualValue);

	void InitWithRecommendedSettings(int width, int height);

	void QueryOptimalSettings(
		DirectX::XMUINT2 inDisplaySize, 
		NVSDK_NGX_PerfQuality_Value inQualValue, 
		DlssRecommendedSettings* outRecommendedSettings);

	void EvaluateSuperSampling(
		CommandList* commandList,
		const DlssTextures& texs,
		int width, int height,
		const Camera& cam,
		bool bResetAccumulation = false,
		float flSharpness = 0.0f,
		bool bUseNgxSdkExtApi = false);


	void OnGUI();

	DirectX::XMUINT2 OnResize(int width, int height);

	void ReleaseDLSSFeatures();

	void ShutDown();

	bool bDlssOn = false;
	int list_item = 0;

	Texture resolvedTexture{};

	SRVHeapData heap = SRVHeapData(1);

	NVSDK_NGX_Parameter* m_ngxParameters{ nullptr };
	NVSDK_NGX_Handle* m_dlssFeature{ nullptr };

	DlssRecommendedSettings recommendedSettings{};
	NVSDK_NGX_PerfQuality_Value qualityMode = NVSDK_NGX_PerfQuality_Value::NVSDK_NGX_PerfQuality_Value_Balanced;

	event::event<void(const bool&, const NVSDK_NGX_PerfQuality_Value&)> dlssChangeEvent;
};