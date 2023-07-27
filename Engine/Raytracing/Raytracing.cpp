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

Raytracing::Raytracing()
{
    //auto* assetManager = Application::Get().GetAssetManager();
    //assetManager->AttachMeshCreatedEvent(&Raytracing::OnMeshCreated, this);
}

void Raytracing::BuildAccelerationStructure(CommandList& dxrCommandList, std::vector<MeshInstanceWrapper>& meshWrapperInstances, MeshManager& meshManager, UINT backbufferIndex)
{
    if (!bUpdate) return;

    m_CurrentBufferIndex = backbufferIndex;

    auto device = Application::Get().GetDevice();
  
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs;

    if (meshWrapperInstances.empty()) return;

    auto& meshes = Application::Get().GetAssetManager()->m_MeshManager->GetMeshData();

    for (auto[trans, mesh, material] : meshWrapperInstances)
    {
        auto meshID = MeshInstanceAccess::GetMeshID(mesh);
        const auto& internalMesh = meshes[meshID];

        if (!internalMesh.blas) continue;

        auto transform = trans.GetTransform();

        D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
       
        instanceDesc.InstanceID = meshID;

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

      //  if (material.HasOpacity())
       // {
        //    instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE;
       //     instanceDesc.InstanceMask |= INSTANCE_TRANSLUCENT;
       // } 
        /*
        else if (mesh.IsPointlight())
        {
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
            instanceDesc.InstanceMask = INSTANCE_LIGHT;
        }
        */
       // else
       // {
            instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
            instanceDesc.InstanceMask |= INSTANCE_OPAQUE;
       // }
        
        instanceDesc.AccelerationStructure = internalMesh.blas->GetGPUVirtualAddress();
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

//Callback
void Raytracing::OnMeshCreated(const Mesh& mesh)
{
    //TODO: Add meshes to RT structure on creation
}
