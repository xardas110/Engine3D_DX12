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
		TonemapCB,
		Size
	};
}

namespace ExposureParam
{
	enum
	{
		ExposureTex,
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

	void CreateRenderTarget(int nativeWidth, int nativeHeight);

	void OnResize(int nativeWidth, int nativeHeight);

	static void UpdateGUI();

	static TonemapCB& GetTonemapCB();

	//Returns handle to heap start
	D3D12_GPU_DESCRIPTOR_HANDLE CreateUAVViews();

	RenderTarget renderTarget;

	RootSignature rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;

	Texture exposureTex;
	RootSignature exposureRT;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> exposurePSO;

	SRVHeapData heap;
};