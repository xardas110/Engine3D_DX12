#include <EnginePCH.h>
#include <Raytracing.h>
#include <Application.h>
#include <CommandList.h>
#include <CommandQueue.h>
#include <d3dx12.h>
#include <MeshManager.h>
#include <Components.h>

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#define DEBUG_DXR

void AllocateUAVBuffer(ID3D12Device* pDevice, UINT64 bufferSize, ID3D12Resource** ppResource, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON, const wchar_t* resourceName = nullptr)
{
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        initialResourceState,
        nullptr,
        IID_PPV_ARGS(ppResource)));
    if (resourceName)
    {
        (*ppResource)->SetName(resourceName);
    }
}

void AllocateUploadBuffer(ID3D12Device* pDevice, void* pData, UINT64 datasize, ID3D12Resource** ppResource, const wchar_t* resourceName = nullptr)
{
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(datasize);
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(ppResource)));
    if (resourceName)
    {
        (*ppResource)->SetName(resourceName);
    }
    void* pMappedData;
    (*ppResource)->Map(0, nullptr, &pMappedData);
    memcpy(pMappedData, pData, datasize);
    (*ppResource)->Unmap(0, nullptr);
}

void Raytracing::Init()
{
    auto& meshData = Application::Get().GetAssetManager()->m_MeshManager.meshData;

    meshData.meshCreationEvent.attach(&Raytracing::OnMeshCreated, this);

}

void Raytracing::BuildAccelerationStructures()
{
    /*
    auto device = Application::Get().GetDevice();
    auto commandQueue = Application::Get().GetCommandQueue();
    auto commandList = commandQueue->GetCommandList();

    auto& bottomLevelAccelerationStructure = m_RaytracingAccelerationStructure.bottomLevelAccelerationStructure;
    auto& topLevelAccelerationStructure = m_RaytracingAccelerationStructure.topLevelAccelerationStructure;

    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.IndexBuffer = m_Cube->m_IndexBuffer.GetD3D12Resource()->GetGPUVirtualAddress();
    geometryDesc.Triangles.IndexCount = m_Cube->m_IndexCount;
    geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
    geometryDesc.Triangles.Transform3x4 = 0;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.VertexCount = static_cast<UINT>(m_Cube->m_VertexBuffer.GetD3D12Resource()->GetDesc().Width) / sizeof(VertexPositionNormalTexture);
    geometryDesc.Triangles.VertexBuffer.StartAddress = m_Cube->m_VertexBuffer.GetD3D12Resource()->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(VertexPositionNormalTexture);
    
    // Mark the geometry as opaque. 
    // PERFORMANCE TIP: mark geometry as opaque whenever applicable as it can enable important ray processing optimizations.
    // Note: When rays encounter opaque geometry an any hit shader will not be executed whether it is present or not.
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    // Get required sizes for an acceleration structure.
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelBuildDesc.Inputs;
    bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bottomLevelInputs.Flags = buildFlags;
    bottomLevelInputs.NumDescs = 1;
    bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bottomLevelInputs.pGeometryDescs = &geometryDesc;
    
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = buildFlags;
    topLevelInputs.NumDescs = 1;
    topLevelInputs.pGeometryDescs = nullptr;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;


    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
    ThrowIfFalse(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0, L" ");

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
    ThrowIfFalse(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0, L"L");

    ComPtr<ID3D12Resource> scratchResource;
    AllocateUAVBuffer(device.Get(), max(topLevelPrebuildInfo.ScratchDataSizeInBytes, bottomLevelPrebuildInfo.ScratchDataSizeInBytes), &scratchResource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"ScratchResource");

    // Allocate resources for acceleration structures.
    // Acceleration structures can only be placed in resources that are created in the default heap (or custom heap equivalent). 
    // Default heap is OK since the application doesn’t need CPU read/write access to them. 
    // The resources that will contain acceleration structures must be created in the state D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, 
    // and must have resource flag D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS. The ALLOW_UNORDERED_ACCESS requirement simply acknowledges both: 
    //  - the system will be doing this type of access in its implementation of acceleration structure builds behind the scenes.
    //  - from the app point of view, synchronization of writes/reads to acceleration structures is accomplished using UAV barriers.
    {
        D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

        AllocateUAVBuffer(device.Get(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, &bottomLevelAccelerationStructure, initialResourceState, L"BottomLevelAccelerationStructure");
        AllocateUAVBuffer(device.Get(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &topLevelAccelerationStructure, initialResourceState, L"TopLevelAccelerationStructure");
    }

    // Create an instance desc for the bottom-level acceleration structure.
    ComPtr<ID3D12Resource> instanceDescs;
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.InstanceID = 1;
    instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
    instanceDesc.InstanceMask = 1;
    instanceDesc.AccelerationStructure = bottomLevelAccelerationStructure->GetGPUVirtualAddress();
    AllocateUploadBuffer(device.Get(), &instanceDesc, sizeof(instanceDesc), &instanceDescs, L"InstanceDescs");

    // Bottom Level Acceleration Structure desc
    {
        bottomLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
        bottomLevelBuildDesc.DestAccelerationStructureData = bottomLevelAccelerationStructure->GetGPUVirtualAddress();
    }

    // Top Level Acceleration Structure desc
    {
        topLevelBuildDesc.DestAccelerationStructureData = topLevelAccelerationStructure->GetGPUVirtualAddress();
        topLevelBuildDesc.ScratchAccelerationStructureData = scratchResource->GetGPUVirtualAddress();
        topLevelBuildDesc.Inputs.InstanceDescs = instanceDescs->GetGPUVirtualAddress();
    }

    auto BuildAccelerationStructure = [&](auto* raytracingCommandList)
    {
        raytracingCommandList->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);
        commandList->GetGraphicsCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(bottomLevelAccelerationStructure.Get()));
        raytracingCommandList->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
    };

    // Build acceleration structure.
    BuildAccelerationStructure(commandList->GetGraphicsCommandList().Get());

    auto fenceVal = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceVal);
    */
}

void Raytracing::BuildAccelerationStructure(CommandList& dxrCommandList, std::vector<MeshInstanceWrapper>& meshWrapperInstances, MeshManager& meshManager, UINT backbufferIndex)
{
    if (!bUpdate) return;

    m_CurrentBufferIndex = backbufferIndex;

    auto device = Application::Get().GetDevice();
  
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;

    auto& meshes = meshManager.meshData.meshes;
    auto& meshInstances = meshManager.instanceData;

    if (meshWrapperInstances.empty()) return;

    for (auto[trans, mesh] : meshWrapperInstances)
    {
        const auto& internalMesh = meshes[meshInstances.meshIds[mesh.id]];

        if (!internalMesh.mesh.blas) continue;

        auto transform = trans.GetTransform();

        D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
       
        instanceDesc.InstanceID = mesh.id;
        instanceDesc.Transform[0][0] = transform.r[0].m128_f32[0];
        instanceDesc.Transform[1][0] = transform.r[0].m128_f32[1];
        instanceDesc.Transform[2][0] = transform.r[0].m128_f32[2];

        instanceDesc.Transform[0][1] = transform.r[1].m128_f32[0];
        instanceDesc.Transform[1][1] = transform.r[1].m128_f32[1];
        instanceDesc.Transform[2][1] = transform.r[1].m128_f32[2];

        instanceDesc.Transform[0][2] = transform.r[2].m128_f32[0];
        instanceDesc.Transform[1][2] = transform.r[2].m128_f32[1];
        instanceDesc.Transform[2][2] = transform.r[2].m128_f32[2];

        instanceDesc.Transform[0][3] = transform.r[3].m128_f32[0];
        instanceDesc.Transform[1][3] = transform.r[3].m128_f32[1];
        instanceDesc.Transform[2][3] = transform.r[3].m128_f32[2];

        if (mesh.HasOpacity())
        {
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE;
            instanceDesc.InstanceMask |= INSTANCE_TRANSLUCENT;
        } 
        else if (mesh.IsPointlight())
        {
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
            instanceDesc.InstanceMask |= INSTANCE_LIGHT;
        }
        else
        {
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
            instanceDesc.InstanceMask |= INSTANCE_OPAQUE;
        }

        instanceDesc.AccelerationStructure = internalMesh.mesh.blas->GetGPUVirtualAddress();
        instanceDescs.emplace_back(instanceDesc);
    }

    auto byteSize = instanceDescs.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
    AllocateUploadBuffer(device.Get(), instanceDescs.data(), byteSize, &m_InstanceResource[m_CurrentBufferIndex], L"InstanceDescs");

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelBuildDesc.Inputs;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    topLevelInputs.Flags = buildFlags;
    topLevelInputs.NumDescs = instanceDescs.size();
    topLevelInputs.pGeometryDescs = nullptr;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
    device->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
    ThrowIfFalse(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0, L" ");

    AllocateUAVBuffer(device.Get(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, &m_TopLevelAccelerationStructure[m_CurrentBufferIndex], D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"TopLevelAccelerationStructure");
   
    AllocateUAVBuffer(device.Get(), topLevelPrebuildInfo.ScratchDataSizeInBytes, &m_TopLevelScratch[m_CurrentBufferIndex], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"ScratchResource");

    topLevelBuildDesc.DestAccelerationStructureData = m_TopLevelAccelerationStructure[m_CurrentBufferIndex]->GetGPUVirtualAddress();
    topLevelBuildDesc.ScratchAccelerationStructureData = m_TopLevelScratch[m_CurrentBufferIndex]->GetGPUVirtualAddress();
    topLevelBuildDesc.Inputs.InstanceDescs = m_InstanceResource[m_CurrentBufferIndex]->GetGPUVirtualAddress();
   
    dxrCommandList.GetGraphicsCommandList()->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
}

void Raytracing::OnMeshCreated(const Mesh& mesh)
{

}
