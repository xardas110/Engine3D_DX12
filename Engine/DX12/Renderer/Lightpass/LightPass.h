#pragma once
#pragma once
#include <RenderTarget.h>
#include <RootSignature.h>
#include <CommandList.h>

namespace LightPassParam
{
	enum
	{
		GlobalHeapData,
		GlobalMatInfo,
		GlobalMaterials,
		GlobalMeshInfo,
		GBuffer,
		Size
	};
}

struct LightPass
{
private:
	friend class DeferredRenderer;
	friend class Window;

	LightPass(const int& width, const int& height);
	void CreateRenderTarget(int width, int height);
	void CreatePipeline();

	void ClearRendetTarget(CommandList& commandlist, float clearColor[4]);

	void OnResize(int width, int height);

	RenderTarget renderTarget;
	RootSignature rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
};