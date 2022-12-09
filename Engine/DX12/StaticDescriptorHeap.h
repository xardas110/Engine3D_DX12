#pragma once
#include <d3d12.h>
#include <wrl.h>

struct SRVHeapData
{
	friend class TextureManager;
	friend class MeshManager;
	friend class DeferredRenderer;
	friend class GUI;
private:
	mutable size_t lastIndex = 0;
	const size_t increment = 0;
	const int heapSize = 50000;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
public:
	SRVHeapData();
	SRVHeapData(int heapSize);

	//Call this only once per resource!
	D3D12_CPU_DESCRIPTOR_HANDLE SetHandle(UINT srvIndex)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += srvIndex * increment;
		return handle;
	}

	//Call this only once per resource!
	D3D12_CPU_DESCRIPTOR_HANDLE IncrementHandle(UINT& outCurrentHandleIndex) const
	{
		outCurrentHandleIndex = lastIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += lastIndex++ * increment;
		return handle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetHandleAtStart()
	{
		return heap->GetGPUDescriptorHandleForHeapStart();
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(UINT index)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE handle = heap->GetGPUDescriptorHandleForHeapStart();
		handle.ptr += index * increment;
		return handle;
	}

	void Resethandle(int resetOffset)
	{
		lastIndex = resetOffset;
	}
};