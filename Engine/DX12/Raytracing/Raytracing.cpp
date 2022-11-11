#include <EnginePCH.h>
#include <Raytracing.h>
#include <Application.h>
#include <CommandList.h>
#include <CommandQueue.h>
#include <d3dx12.h>

void Raytracing::Init()
{
	CreateRaytracingInterfaces();
    CreateRootSignature();
}

void Raytracing::CreateRaytracingInterfaces()
{
    auto device = Application::Get().GetDevice();
    auto cq = Application::Get().GetCommandQueue();
    auto cl = cq->GetCommandList();

    ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(&m_DxrDevice)), L"Couldn't get DirectX Raytracing interface for the device.\n");
    ThrowIfFailed(cl->GetGraphicsCommandList()->QueryInterface(IID_PPV_ARGS(&m_DxrCommandlist)), L"Couldn't get DirectX Raytracing interface for the command list.\n");
}

void Raytracing::CreateRootSignature()
{
    auto device = Application::Get().GetDevice();

    // Global Root Signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    {
        CD3DX12_DESCRIPTOR_RANGE ranges[2]; // Perfomance TIP: Order from most frequent to least frequent.
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);  // 1 output texture
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1);  // 2 static index and vertex buffers.

        CD3DX12_ROOT_PARAMETER rootParameters[GlobalRootSignatureParams::Count];
        rootParameters[GlobalRootSignatureParams::OutputViewSlot].InitAsDescriptorTable(1, &ranges[0]);
        rootParameters[GlobalRootSignatureParams::AccelerationStructureSlot].InitAsShaderResourceView(0);
        rootParameters[GlobalRootSignatureParams::SceneConstantSlot].InitAsConstantBufferView(0);
        rootParameters[GlobalRootSignatureParams::VertexBuffersSlot].InitAsDescriptorTable(1, &ranges[1]);
        CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);

        ComPtr<ID3DBlob> blob;
        ComPtr<ID3DBlob> error;

        ThrowIfFailed(D3D12SerializeRootSignature(&globalRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
        ThrowIfFailed(device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(m_raytracingGlobalRootSignature))));
    }

    // Local Root Signature
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    {
        ComPtr<ID3DBlob> blob;
        ComPtr<ID3DBlob> error;

        CD3DX12_ROOT_PARAMETER rootParameters[LocalRootSignatureParams::Count];
        rootParameters[LocalRootSignatureParams::CubeConstantSlot].InitAsConstants(SizeOfInUint32(m_CubeCB), 1);
        CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
        localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

        ThrowIfFailed(D3D12SerializeRootSignature(&localRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error), error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr);
        ThrowIfFailed(device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(m_raytracingGlobalRootSignature))));
    }
}

void Raytracing::CreateRaytracingPipelineStateObject()
{
    CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };


}