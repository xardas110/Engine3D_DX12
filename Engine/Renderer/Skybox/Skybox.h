#pragma once
#include <Texture.h>
#include <RootSignature.h>
#include <StaticDescriptorHeap.h>

struct Skybox
{
private:
	friend class DeferredRenderer;

	Skybox(const std::wstring& panoPath);

	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVView();

	Texture panoTexture;
	Texture cubemap;

	SRVHeapData heap;
};