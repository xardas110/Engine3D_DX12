#pragma once
#include <TypesCompat.h>
#include <RootSignature.h>
#include <RenderTarget.h>
#include <StaticDescriptorHeap.h>

namespace HDRParam
{
	enum
	{
		ColorTexture,
		ExposureTex,
		TonemapCB,
		Size
	};
}

namespace ExposureParam
{
	enum
	{
		ExposureTex,
		Histogram,
		TonemapCB,
		Size
	};
}

namespace HistogramParam
{
	enum
	{
		SourceTex,
		Histogram,
		TonemapCB,
		Size
	};
}

struct EyeAdaption
{
	float lowPercentile = 0.8f;
	float highPercentile = 0.95f;
	float eyeAdaptionSpeedUp = 1.0f;
	float eyeAdaptionSpeedDown = 5.0f;
	float minAdaptedLuminance = 0.1f;
	float maxAdaptedLuminance = 5.f;
	float exposureBias = 0.0f;
	float whitePoint = 3.f;
};

struct HDR
{
private:
	friend class DeferredRenderer;

	HDR(int nativeWidth, int nativeHeight);

	void CreatePipeline();

	void CreateExposurePSO();
	void CreateHistogramPSO();

	void CreateRenderTarget(int nativeWidth, int nativeHeight);

	void OnResize(int nativeWidth, int nativeHeight);

	void UpdateGUI();
	void UpdateEyeAdaptionGUI();

	TonemapCB& GetTonemapCB();

	//Returns handle to heap start
	void CreateUAVViews();

	RenderTarget renderTarget;

	RootSignature rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;

	Texture exposureTex;
	Texture histogram;

	RootSignature exposureRT;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> exposurePSO;

	RootSignature histogramRT;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> histogramPSO;

	SRVHeapData exposureHeap;

	SRVHeapData histogramHeap = SRVHeapData(1);
	SRVHeapData histogramClearHeap = SRVHeapData(1, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	EyeAdaption eyeAdaption;
	TonemapCB tonemapParameters;
};