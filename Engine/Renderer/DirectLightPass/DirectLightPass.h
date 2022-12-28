#pragma once

#include <RenderTarget.h>
#include <RootSignature.h>
#include <CommandList.h>
#include <StaticDescriptorHeap.h>

namespace DirectLightPassParam
{
	enum
	{
		GBufferHeap,
		DirectionalLightCB,
		CameraCB,
		Cubemap,
		Size
	};
}

namespace DirectLightShadowPassParam
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
		Size
	};
}

struct DirectLightShadowSettings
{
private:
	enum ScaleTypes
	{
		Full,
		Half,
		Quart,
		Size
	} scale = Full;

	DirectX::XMFLOAT2 scaleValues[ScaleTypes::Size] =
	{
		{1.f, 1.f},
		{0.5f, 1.f},
		{0.5f, 0.5f}
	};
public:
	const DirectX::XMFLOAT2& GetScale() const
	{
		return scaleValues[scale];
	}

	void SetScale(ScaleTypes type)
	{
		if (type == scale) return;
		scale = type;
	}
};

struct DirectLightPass
{
private:
	friend class DeferredRenderer;
	friend class Window;
	friend class NvidiaDenoiser;

	DirectLightPass(const int& width, const int& height);

	void CreateRenderTargets(int width, int height);

	void CreateDirectLightPipeline();
	void CreateDirectLightShadowPipeline();

	void ClearRendetTargets(CommandList& commandlist, float clearColor[4]);

	void OnResize(int width, int height);

	RenderTarget directLightRT;
	RootSignature directLightRS;

	RenderTarget directLightShadowRT;
	RootSignature directLightShadowRS;

	SRVHeapData shadowHeap;
	SRVHeapData directLightHeap;

	//Returns handle to heap start
	void CreateSRVViews();

	Microsoft::WRL::ComPtr<ID3D12PipelineState> directLightPSO;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> directLightShadowPSO;

	DirectLightShadowSettings shadowSettings;
};