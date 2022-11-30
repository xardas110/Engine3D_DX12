#pragma once
#include <TypesCompat.h>
#include <RootSignature.h>
#include <RenderTarget.h>

namespace HDRParam
{
	enum
	{
		ColorTexture,
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

	void CreateRenderTarget(int nativeWidth, int nativeHeight);

	void OnResize(int nativeWidth, int nativeHeight);

	static void UpdateGUI();

	static TonemapCB& GetTonemapCB();

	RootSignature rootSignature;
	RenderTarget renderTarget;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
};