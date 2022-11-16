#pragma once
#include <RenderTarget.h>
#include <RootSignature.h>
#include <CommandList.h>

namespace GBufferParam
{
	enum
	{
		ObjectCB,		
		Textures,
		Size
	};
}

struct GBuffer
{
private:
	friend class DeferredRenderer;
	friend class Window;

	GBuffer(const int& width, const int& height);
	void CreateRenderTarget();
	void CreatePipeline();

	void ClearRendetTarget(CommandList& commandlist, float clearColor[4]);

	void OnResize();

	const int& width, height;
	RenderTarget renderTarget;
	RootSignature rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
};