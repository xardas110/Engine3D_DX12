#include <EnginePCH.h>
#include <Lightpass/LightPass.h>
#include <Application.h>

#include <LightPass_VS.h>
#include <LightPass_PS.h>


LightPass::LightPass(const int& width, const int& height)
{
    CreateRenderTarget(width, height);
    CreatePipeline();
}

void LightPass::CreateRenderTarget(int width, int height)
{
    // Create an HDR intermediate render target.
    DXGI_FORMAT directDiffuseFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT indirectDiffuseFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT indirectSpecularFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    // Create an off-screen render target with a single color buffer and a depth buffer.
    auto directDiffuseDesc = CD3DX12_RESOURCE_DESC::Tex2D(directDiffuseFormat, width, height);
    directDiffuseDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    directDiffuseDesc.MipLevels = 1;

    auto indirectDiffuseDesc = CD3DX12_RESOURCE_DESC::Tex2D(indirectDiffuseFormat, width, height);
    indirectDiffuseDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    indirectDiffuseDesc.MipLevels = 1;

    auto indirectSpecularDesc = CD3DX12_RESOURCE_DESC::Tex2D(indirectSpecularFormat, width, height);
    indirectSpecularDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    indirectSpecularDesc.MipLevels = 1;

    D3D12_CLEAR_VALUE directDiffuseClear;
    directDiffuseClear.Format = directDiffuseFormat;
    directDiffuseClear.Color[0] = 1.0f;
    directDiffuseClear.Color[1] = 1.0f;
    directDiffuseClear.Color[2] = 1.0f;
    directDiffuseClear.Color[3] = 1.0f;

    D3D12_CLEAR_VALUE indirectDiffuseClear;
    indirectDiffuseClear.Format = indirectDiffuseFormat;
    indirectDiffuseClear.Color[0] = 1.0f;
    indirectDiffuseClear.Color[1] = 1.0f;
    indirectDiffuseClear.Color[2] = 1.0f;
    indirectDiffuseClear.Color[3] = 1.0f;

    D3D12_CLEAR_VALUE specularClear;
    specularClear.Format = indirectSpecularFormat;
    specularClear.Color[0] = 1.0f;
    specularClear.Color[1] = 1.0f;
    specularClear.Color[2] = 1.0f;
    specularClear.Color[3] = 1.0f;
  
    Texture directDiffuse = Texture(directDiffuseDesc, &directDiffuseClear,
        TextureUsage::RenderTarget,
        L"LightPass DirectDiffuse");

    Texture indirectDiffuse = Texture(indirectDiffuseDesc, &indirectDiffuseClear,
        TextureUsage::RenderTarget,
        L"LightPass IndirectDiffuse");

    Texture indirectSpecular = Texture(indirectSpecularDesc, &specularClear,
        TextureUsage::RenderTarget,
        L"LightPass IndirectSpecular");

    renderTarget.AttachTexture(AttachmentPoint::Color0, directDiffuse);
    renderTarget.AttachTexture(AttachmentPoint::Color1, indirectDiffuse);
    renderTarget.AttachTexture(AttachmentPoint::Color2, indirectSpecular);
}

void LightPass::CreatePipeline()
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
    srvHeapRanges[0].NumDescriptors = 512;
    srvHeapRanges[0].BaseShaderRegister = 1;
    srvHeapRanges[0].RegisterSpace = 0;
    srvHeapRanges[0].OffsetInDescriptorsFromTableStart = 0;
    srvHeapRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    srvHeapRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvHeapRanges[1].NumDescriptors = 250;
    srvHeapRanges[1].BaseShaderRegister = 1;
    srvHeapRanges[1].RegisterSpace = 1;
    srvHeapRanges[1].OffsetInDescriptorsFromTableStart = 0;
    srvHeapRanges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    srvHeapRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvHeapRanges[2].NumDescriptors = 250;
    srvHeapRanges[2].BaseShaderRegister = 1;
    srvHeapRanges[2].RegisterSpace = 2;
    srvHeapRanges[2].OffsetInDescriptorsFromTableStart = 0;
    srvHeapRanges[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
 
    CD3DX12_DESCRIPTOR_RANGE1 gBufferSRVHeap = {};
    gBufferSRVHeap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    gBufferSRVHeap.NumDescriptors = 5;
    gBufferSRVHeap.BaseShaderRegister = 5;
    gBufferSRVHeap.RegisterSpace = 6;
    gBufferSRVHeap.OffsetInDescriptorsFromTableStart = 0;
    gBufferSRVHeap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    CD3DX12_ROOT_PARAMETER1 rootParameters[LightPassParam::Size];
    rootParameters[LightPassParam::AccelerationStructure].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);
    rootParameters[LightPassParam::GBufferHeap].InitAsDescriptorTable(1, &gBufferSRVHeap);
    rootParameters[LightPassParam::GlobalHeapData].InitAsDescriptorTable(3, srvHeapRanges);
    rootParameters[LightPassParam::GlobalMeshInfo].InitAsShaderResourceView(2, 3);
    rootParameters[LightPassParam::GlobalMatInfo].InitAsShaderResourceView(3, 4);
    rootParameters[LightPassParam::GlobalMaterials].InitAsShaderResourceView(4, 5);     
    CD3DX12_STATIC_SAMPLER_DESC samplers[2];
    samplers[0] = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT);
    samplers[1] = CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
    
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Init_1_1(LightPassParam::Size,
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

    pipelineStateStream.VS = { g_LightPass_VS, sizeof(g_LightPass_VS) };
    pipelineStateStream.PS = { g_LightPass_PS, sizeof(g_LightPass_PS) };

    pipelineStateStream.RTVFormats = renderTarget.GetRenderTargetFormats();

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipeline)));
}

void LightPass::ClearRendetTarget(CommandList& commandlist, float clearColor[4])
{
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color0), clearColor);
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color1), clearColor);
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color2), clearColor);
}

void LightPass::OnResize(int width, int height)
{
    renderTarget.Resize(width, height);
}

D3D12_GPU_DESCRIPTOR_HANDLE LightPass::CreateSRVViews()
{
    auto device = Application::Get().GetDevice();

    device->CreateShaderResourceView(
        renderTarget.GetTexture(AttachmentPoint::Color0).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(0));

    device->CreateShaderResourceView(
        renderTarget.GetTexture(AttachmentPoint::Color1).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(1));

    device->CreateShaderResourceView(
        renderTarget.GetTexture(AttachmentPoint::Color2).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(2));

    return m_SRVHeap.GetHandleAtStart();
}
