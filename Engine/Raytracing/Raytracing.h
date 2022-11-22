#pragma once

#include <TypesCompat.h>
#include <Resource.h>
#include <Mesh.h>
#include <entt/entt.hpp>

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

    // Build raytracing acceleration structures from the generated geometry.
    void BuildAccelerationStructures();

    void Raytracing::BuildAccelerationStructure(CommandList& dxrCommandList, std::vector<MeshInstanceWrapper>& meshWrapperInstances, MeshManager& meshManager, UINT backbufferIndex);

    //Called when a new mesh is created, to insert into BLAS
    void OnMeshCreated(const Mesh& mesh);

    std::unique_ptr<Mesh> m_Cube;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_TopLevelAccelerationStructure[3];
    Microsoft::WRL::ComPtr<ID3D12Resource> m_TopLevelScratch[3];
    Microsoft::WRL::ComPtr<ID3D12Resource> m_InstanceResource[3];

    auto GetCurrentTLAS()
    {
        return m_TopLevelAccelerationStructure[m_CurrentBufferIndex];
    }

    UINT m_CurrentBufferIndex = 0;
};