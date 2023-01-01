#include <EnginePCH.h>
#include <StaticDescriptorHeap.h>
#include <Application.h>

SRVHeapData::SRVHeapData(D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	: increment(Application::Get().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))
{
	auto device = Application::Get().GetDevice();
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = heapSize;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = flags;

	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
}

SRVHeapData::SRVHeapData(int heapSize, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	: increment(Application::Get().GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)), heapSize(heapSize)
{
	auto device = Application::Get().GetDevice();
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.NumDescriptors = heapSize;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = flags;

	device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
}