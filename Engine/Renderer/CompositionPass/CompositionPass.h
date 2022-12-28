#pragma once
#include <RenderTarget.h>
#include <RootSignature.h>
#include <CommandList.h>
#include <StaticDescriptorHeap.h>

namespace CompositionPassParam
{
	enum
	{
		AccelerationStructure,
		GBufferHeap,
		LightMapHeap,
		DirectLightHeap,
		DirectLightShadowHeap,
		GlobalHeapData,
		GlobalMatInfo,
		GlobalMaterials,
		GlobalMeshInfo,
		RaytracingDataCB,
		CameraCB,
		TonemapCB,
		Size
	};
}

struct CompositionPass
{
private:
	friend class DeferredRenderer;
	friend class Window;

	CompositionPass(const int& width, const int& height);
	void CreateRenderTarget(int width, int height);
	void CreatePipeline();

	void ClearRendetTarget(CommandList& commandlist, float clearColor[4]);

	void OnResize(int width, int height);

	//Returns handle to heap start
	D3D12_GPU_DESCRIPTOR_HANDLE CreateSRVViews();

	SRVHeapData heap;

	RenderTarget renderTarget;
	RootSignature rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
};