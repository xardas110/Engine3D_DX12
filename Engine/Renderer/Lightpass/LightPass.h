#pragma once
#pragma once
#include <RenderTarget.h>
#include <RootSignature.h>
#include <CommandList.h>
#include <StaticDescriptorHeap.h>

namespace LightPassParam
{
	enum
	{
		AccelerationStructure,
		GBufferHeap,
		GlobalHeapData,
		GlobalMatInfo,
		GlobalMaterials,
		GlobalMeshInfo,
		DirectionalLightCB,
		CameraCB,
		RaytracingDataCB,
		AccumBuffer,
		Size
	};
}

struct LightPass
{
private:
	friend class DeferredRenderer;
	friend class Window;
	friend class NvidiaDenoiser;

	LightPass(const int& width, const int& height);
	void CreateRenderTarget(int width, int height);
	void CreatePipeline();

	void ClearRendetTarget(CommandList& commandlist, float clearColor[4]);

	void OnResize(int width, int height);

	//Returns handle to heap start
	D3D12_GPU_DESCRIPTOR_HANDLE CreateSRVViews();

	//Returns handle to heap start
	D3D12_GPU_DESCRIPTOR_HANDLE CreateUAVViews();

	SRVHeapData m_SRVHeap;

	RenderTarget renderTarget;
	RootSignature rootSignature;

	Texture rwAccumulation;
	Texture denoisedIndirectDiffuse;
	Texture denoisedIndirectSpecular;

	const Texture& GetTexture(int type);

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
};