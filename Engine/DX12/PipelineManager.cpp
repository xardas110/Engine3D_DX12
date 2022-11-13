#include <EnginePCH.h>
#include <PipelineManager.h>
#include <Application.h>

#include <GeometryMesh_VS.h>
#include <GeometryMesh_PS.h>

#include <Camera.h>

PipelineManager::PipelineManager()
{
	CreateGeometryMeshPSO();
}

PipelineManager::~PipelineManager()
{
	for (size_t i = 0; i < Pipeline::Size; i++)
	{
		m_Pipelines[i].pipelineRef = nullptr;
	}
}

std::unique_ptr<PipelineManager> PipelineManager::CreateInstance()
{
    return std::unique_ptr<PipelineManager>(new PipelineManager);
}

void PipelineManager::CreateGeometryMeshPSO()
{
    auto device = Application::Get().GetDevice();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE1 srvRanges[2] = {};
    srvRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRanges[0].NumDescriptors = 512;
    srvRanges[0].BaseShaderRegister = 1;
    srvRanges[0].RegisterSpace = 0;
    srvRanges[0].OffsetInDescriptorsFromTableStart = 0;
    srvRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    srvRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRanges[1].NumDescriptors = 512;
    srvRanges[1].BaseShaderRegister = 1;
    srvRanges[1].RegisterSpace = 1;
    srvRanges[1].OffsetInDescriptorsFromTableStart = 0;
    srvRanges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    CD3DX12_ROOT_PARAMETER1 rootParameters[GlobalRootParam::Size];
    rootParameters[GlobalRootParam::ObjectCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[GlobalRootParam::AccelerationStructure].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);
    rootParameters[GlobalRootParam::GlobalHeapData].InitAsDescriptorTable(2, srvRanges);
    rootParameters[GlobalRootParam::GlobalMeshInfo].InitAsShaderResourceView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);
    rootParameters[GlobalRootParam::GlobalMaterialInfo].InitAsShaderResourceView(3, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);

    CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Init_1_1(GlobalRootParam::Size,
        rootParameters, 1, &linearRepeatSampler,
        rootSignatureFlags);

    auto& rootSignature = m_Pipelines[Pipeline::GeometryMesh].rootSignature;

    rootSignature.SetRootSignatureDesc(
        rootSignatureDesc.Desc_1_1,
        featureData.HighestVersion
    );

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    } pipelineStateStream;

    pipelineStateStream.pRootSignature = rootSignature.GetRootSignature().Get();

    pipelineStateStream.InputLayout = { VertexPositionNormalTextureTangentBitangent::InputElements ,VertexPositionNormalTextureTangentBitangent::InputElementCount };

    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    pipelineStateStream.VS = { g_GeometryMesh_VS, sizeof(g_GeometryMesh_VS) };
    pipelineStateStream.PS = { g_GeometryMesh_PS, sizeof(g_GeometryMesh_PS) };

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_Pipelines[Pipeline::GeometryMesh].pipelineRef)));
}
