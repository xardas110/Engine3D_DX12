#pragma once
#include <RenderTarget.h>
#include <RootSignature.h>
#include <CommandList.h>

namespace GBufferParam
{
	enum
	{
		ObjectCB,		
		GlobalHeapData,
		GlobalMatInfo,
		GlobalMaterials,
		Size
	};
}

namespace DepthPrePassParam
{
	enum
	{
		ObjectCB,
		Size
	};
}

struct GBuffer
{
private:
	friend class DeferredRenderer;
	friend class Window;

	GBuffer(const int& width, const int& height);
	void CreateRenderTarget(int width, int height);
	void CreatePipeline();
	void CreateDepthPrePassPipeline();

	void ClearRendetTarget(CommandList& commandlist, float clearColor[4]);

	void OnResize(int width, int height);

	RenderTarget renderTarget;
	RootSignature rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;

	RootSignature zPrePassRS;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> zPrePassPipeline;

	UINT albedoIndex, normalIndex, pbrIndex, emissiveIndex;
};