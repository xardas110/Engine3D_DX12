#pragma once
#include <RenderTarget.h>
#include <RootSignature.h>
#include <CommandList.h>

namespace CompositionPassParam
{
	enum
	{
		AccelerationStructure,
		GBufferHeap,
		LightMapHeap,
		GlobalHeapData,
		GlobalMatInfo,
		GlobalMaterials,
		GlobalMeshInfo,
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

	RenderTarget renderTarget;
	RootSignature rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
};