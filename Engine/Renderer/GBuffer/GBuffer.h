#pragma once
#include <RenderTarget.h>
#include <RootSignature.h>
#include <CommandList.h>
#include <StaticDescriptorHeap.h>
#include <TypesCompat.h>

namespace GBufferParam
{
	enum
	{
		ObjectCB,		
		CameraCB,
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

	//See TypesCompat.h for gbuffer textures. i.e GBUFFER_ALBEDO == albedo type
	const Texture& GetTexture(int type);

	RenderTarget renderTarget;

	RootSignature rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;

	RootSignature zPrePassRS;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> zPrePassPipeline;

	SRVHeapData m_SRVHeap;

	//Returns handle to heap start
	D3D12_GPU_DESCRIPTOR_HANDLE CreateSRVViews();
};