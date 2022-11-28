#pragma once

#include <nvsdk_ngx.h>
#include <nvsdk_ngx_helpers.h>

#define APP_ID 231313132

struct DLSS
{
private:
	friend class DeferredRenderer;
	friend struct std::default_delete<DLSS>;

	DLSS();
	~DLSS();

	void InitFeatures(DirectX::XMINT2 optimalRenderSize, 
		DirectX::XMINT2 outDisplaySize, 
		int isContentHDR, bool depthInverted, 
		float depthScale, bool enableSharpening, 
		bool enableAutoExposure, NVSDK_NGX_PerfQuality_Value qualValue);

	void ShutDown();

	NVSDK_NGX_Parameter* m_ngxParameters = nullptr;
	NVSDK_NGX_Handle* m_dlssFeature = nullptr;
};