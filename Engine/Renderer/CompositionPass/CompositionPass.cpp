#include <EnginePCH.h>
#include <CompositionPass.h>

#include <EnginePCH.h>
#include <Lightpass/LightPass.h>
#include <Application.h>

#include <LightPass_VS.h>
#include <LightPass_PS.h>

#include <CompositionPass_VS.h>
#include <CompositionPass_PS.h>

CompositionPass::CompositionPass(const int& width, const int& height)
{
    CreateRenderTarget(width, height);
    CreatePipeline();
}

void CompositionPass::CreateRenderTarget(int width, int height)
{
    DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    // Create an off-screen render target with a single color buffer and a depth buffer.
    auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(colorFormat, width, height);
    colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    colorDesc.MipLevels = 1;

    D3D12_CLEAR_VALUE colorClear;
    colorClear.Format = colorFormat;
    colorClear.Color[0] = 1.0f;
    colorClear.Color[1] = 1.0f;
    colorClear.Color[2] = 1.0f;
    colorClear.Color[3] = 1.0f;

    Texture color = Texture(colorDesc, &colorClear,
        TextureUsage::RenderTarget,
        L"Color buffer");

    renderTarget.AttachTexture(AttachmentPoint::Color0, color);
}

void CompositionPass::CreatePipeline()
{
    auto device = Application::Get().GetDevice();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE1 srvHeapRanges[3] = {};
    srvHeapRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvHeapRanges[0].NumDescriptors = UINT_MAX;
    srvHeapRanges[0].BaseShaderRegister = 1;
    srvHeapRanges[0].RegisterSpace = 0;
    srvHeapRanges[0].OffsetInDescriptorsFromTableStart = 0;
    srvHeapRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    srvHeapRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvHeapRanges[1].NumDescriptors = UINT_MAX;
    srvHeapRanges[1].BaseShaderRegister = 1;
    srvHeapRanges[1].RegisterSpace = 1;
    srvHeapRanges[1].OffsetInDescriptorsFromTableStart = 0;
    srvHeapRanges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    srvHeapRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvHeapRanges[2].NumDescriptors = UINT_MAX;
    srvHeapRanges[2].BaseShaderRegister = 1;
    srvHeapRanges[2].RegisterSpace = 2;
    srvHeapRanges[2].OffsetInDescriptorsFromTableStart = 0;
    srvHeapRanges[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    CD3DX12_DESCRIPTOR_RANGE1 gBufferSRVHeap = {};
    gBufferSRVHeap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    gBufferSRVHeap.NumDescriptors = 20;
    gBufferSRVHeap.BaseShaderRegister = 5;
    gBufferSRVHeap.RegisterSpace = 6;
    gBufferSRVHeap.OffsetInDescriptorsFromTableStart = 0;
    gBufferSRVHeap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    CD3DX12_DESCRIPTOR_RANGE1 lightmapSRVHeap = {};
    lightmapSRVHeap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    lightmapSRVHeap.NumDescriptors = 20;
    lightmapSRVHeap.BaseShaderRegister = 6;
    lightmapSRVHeap.RegisterSpace = 7;
    lightmapSRVHeap.OffsetInDescriptorsFromTableStart = 0;
    lightmapSRVHeap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    CD3DX12_ROOT_PARAMETER1 rootParameters[CompositionPassParam::Size];
    rootParameters[CompositionPassParam::AccelerationStructure].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);
    rootParameters[CompositionPassParam::LightMapHeap].InitAsDescriptorTable(1, &lightmapSRVHeap);
    rootParameters[CompositionPassParam::GBufferHeap].InitAsDescriptorTable(1, &gBufferSRVHeap);
    rootParameters[CompositionPassParam::GlobalHeapData].InitAsDescriptorTable(3, srvHeapRanges);
    rootParameters[CompositionPassParam::GlobalMeshInfo].InitAsShaderResourceView(2, 3);
    rootParameters[CompositionPassParam::GlobalMatInfo].InitAsShaderResourceView(3, 4);
    rootParameters[CompositionPassParam::GlobalMaterials].InitAsShaderResourceView(4, 5);

    CD3DX12_STATIC_SAMPLER_DESC samplers[2];
    samplers[0] = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT);
    samplers[1] = CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Init_1_1(CompositionPassParam::Size,
        rootParameters, 2, samplers);

    rootSignature.SetRootSignatureDesc(
        rootSignatureDesc.Desc_1_1,
        featureData.HighestVersion
    );

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    } pipelineStateStream;

    CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
    pipelineStateStream.Rasterizer = rasterizerDesc;

    pipelineStateStream.pRootSignature = rootSignature.GetRootSignature().Get();

    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    pipelineStateStream.VS = { g_CompositionPass_VS, sizeof(g_CompositionPass_VS) };
    pipelineStateStream.PS = { g_CompositionPass_PS, sizeof(g_CompositionPass_PS) };

    pipelineStateStream.RTVFormats = renderTarget.GetRenderTargetFormats();

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipeline)));
}

void CompositionPass::ClearRendetTarget(CommandList& commandlist, float clearColor[4])
{
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color0), clearColor);
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color1), clearColor);
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color2), clearColor);
}

void CompositionPass::OnResize(int width, int height)
{
    renderTarget.Resize(width, height);
}
