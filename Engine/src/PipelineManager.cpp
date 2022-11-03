#include <EnginePCH.h>
#include <PipelineManager.h>
#include <Application.h>

#include <GeometryMesh_VS.h>
#include <GeometryMesh_PS.h>

#include <Camera.h>

PipelineManager::PipelineManager()
{
	CreateGeometryPSO();
}

PipelineManager::~PipelineManager()
{
	for (size_t i = 0; i < Pipeline::Size; i++)
	{
		m_Pipelines[i].pipelineRef = nullptr;
	}
}

void PipelineManager::CreateGeometryPSO()
{
    auto device = Application::Get().GetDevice();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE1 srcMip(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
    CD3DX12_DESCRIPTOR_RANGE1 outMip(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

    CD3DX12_ROOT_PARAMETER1 rootParameters[GeometryMeshRootParam::Size];
    rootParameters[GeometryMeshRootParam::CameraCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[GeometryMeshRootParam::MaterialCB].InitAsConstantBufferView(0, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc(
        GeometryMeshRootParam::Size,
        rootParameters
    );

    auto& rootSignature = m_Pipelines[Pipeline::GeometryMesh].rootSignature;

    rootSignature.SetRootSignatureDesc(
        rootSignatureDesc.Desc_1_1,
        featureData.HighestVersion
    );

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
    } pipelineStateStream;

    pipelineStateStream.pRootSignature = rootSignature.GetRootSignature().Get();
    pipelineStateStream.VS = { g_GeometryMesh_VS, sizeof(g_GeometryMesh_VS) };
    pipelineStateStream.PS = { g_GeometryMesh_PS, sizeof(g_GeometryMesh_PS) };

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_Pipelines[Pipeline::GeometryMesh].pipelineRef)));
}
