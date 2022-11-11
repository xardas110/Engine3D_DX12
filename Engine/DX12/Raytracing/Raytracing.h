#pragma once

#include <RayTracingHlslCompat.h>

#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)

namespace GlobalRootSignatureParams {
    enum Value {
        OutputViewSlot = 0,
        AccelerationStructureSlot,
        SceneConstantSlot,
        VertexBuffersSlot,
        Count
    };
}

namespace LocalRootSignatureParams {
    enum Value {
        CubeConstantSlot = 0,
        Count
    };
}

class Raytracing
{
	friend class DeferredRenderer;

	void Init();

	void CreateRaytracingInterfaces();
	void CreateRootSignature();
    void CreateRaytracingPipelineStateObject();

    // Create a heap for descriptors.
    void CreateDescriptorHeap();

    // Build geometry to be used in the sample.
    void BuildGeometry();

    // Build raytracing acceleration structures from the generated geometry.
    void BuildAccelerationStructures();

    void CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline);
    void SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, Microsoft::WRL::ComPtr<ID3D12RootSignature>* rootSig);


	Microsoft::WRL::ComPtr<ID3D12Device5> m_DxrDevice;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> m_DxrCommandlist;
    Microsoft::WRL::ComPtr<ID3D12StateObject> m_DxrStateObject;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracingLocalRootSignature;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracingGlobalRootSignature;

    // Geometry
    struct D3DBuffer
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle;
    };
    D3DBuffer m_IndexBuffer;
    D3DBuffer m_VertexBuffer;

    // Acceleration structure
    Microsoft::WRL::ComPtr<ID3D12Resource> m_BottomLevelAccelerationStructure;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_TopLevelAccelerationStructure;

    UINT Raytracing::CreateBufferSRV(D3DBuffer* buffer, UINT numElements, UINT elementSize);

    // Allocate a descriptor and return its index. 
// If the passed descriptorIndexToUse is valid, it will be used instead of allocating a new one.
    UINT AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse = UINT_MAX);


    // Descriptors
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
    UINT m_DescriptorsAllocated{0U};
    UINT m_DescriptorSize{ 0U };

    CubeConstantBuffer m_CubeCB;
};