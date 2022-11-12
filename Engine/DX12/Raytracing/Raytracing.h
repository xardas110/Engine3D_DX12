#pragma once

#include <RayTracingHlslCompat.h>
#include <Resource.h>
#include <Mesh.h>

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

struct RTX : public Resource
{
    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override
    {
        return m_ShaderResourceView;
    }

    virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override
    {
        return m_UavView;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE m_ShaderResourceView;
    D3D12_CPU_DESCRIPTOR_HANDLE m_UavView;
};

class Raytracing
{
	friend class DeferredRenderer;

	void Init();

	void CreateRaytracingInterfaces();

    // Create a heap for descriptors.
    void CreateDescriptorHeap();

    // Build geometry to be used in the sample.
    void BuildGeometry();

    // Build raytracing acceleration structures from the generated geometry.
    void BuildAccelerationStructures();

    std::unique_ptr<Mesh> m_Cube;

    // Acceleration structure
    Microsoft::WRL::ComPtr<ID3D12Resource> m_BottomLevelAccelerationStructure;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_TopLevelAccelerationStructure;

    // Allocate a descriptor and return its index. 
// If the passed descriptorIndexToUse is valid, it will be used instead of allocating a new one.
    //UINT AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse = UINT_MAX);

    // Descriptors
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DescriptorHeap;
    UINT m_DescriptorsAllocated{0U};
    UINT m_DescriptorSize{ 0U };

    CubeConstantBuffer m_CubeCB;
};