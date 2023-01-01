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

	static void UpdateGUI();

	static TonemapCB& GetTonemapCB();

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
};