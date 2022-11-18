#pragma once
#include <RenderTarget.h>
#include <RootSignature.h>
#include <CommandList.h>

namespace DebugTextureParam
{
	enum
	{
		texture,
		Size
	};
}

struct DebugTexturePass
{
private:
	friend class DeferredRenderer;
	friend class Window;

	DebugTexturePass(int width, int height)
	{
		CreatePipeline();
		CreateRenderTarget(width, height);
	}

	void CreatePipeline();
	void CreateRenderTarget(int width, int height);
	void ClearRendetTarget(CommandList& commandlist, float clearColor[4]);

	void OnResize(int width, int height);

	RenderTarget renderTarget;
	RootSignature rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
};