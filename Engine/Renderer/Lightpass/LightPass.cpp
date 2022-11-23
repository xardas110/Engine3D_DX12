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
    DXGI_FORMAT directDiffuseFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    DXGI_FORMAT indirectDiffuseFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    DXGI_FORMAT indirectSpecularFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    DXGI_FORMAT rwAccumFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
    DXGI_FORMAT normalRoughnessFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
    DXGI_FORMAT linearDepthFormat = DXGI_FORMAT_R32_FLOAT;

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

    auto normalRoughnessDesc = CD3DX12_RESOURCE_DESC::Tex2D(normalRoughnessFormat, width, height);
    normalRoughnessDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    normalRoughnessDesc.MipLevels = 1;

    auto linearDepthDesc = CD3DX12_RESOURCE_DESC::Tex2D(linearDepthFormat, width, height);
    linearDepthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    linearDepthDesc.MipLevels = 1;

    auto rwAccumDesc = CD3DX12_RESOURCE_DESC::Tex2D(rwAccumFormat, width, height);
    rwAccumDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    rwAccumDesc.MipLevels = 1;

    D3D12_CLEAR_VALUE directDiffuseClear;
    directDiffuseClear.Format = directDiffuseFormat;
    directDiffuseClear.Color[0] = 0.0f;
    directDiffuseClear.Color[1] = 0.0f;
    directDiffuseClear.Color[2] = 0.0f;
    directDiffuseClear.Color[3] = 0.0f;

    D3D12_CLEAR_VALUE indirectDiffuseClear;
    indirectDiffuseClear.Format = indirectDiffuseFormat;
    indirectDiffuseClear.Color[0] = 0.0f;
    indirectDiffuseClear.Color[1] = 0.0f;
    indirectDiffuseClear.Color[2] = 0.0f;
    indirectDiffuseClear.Color[3] = 0.0f;

    D3D12_CLEAR_VALUE specularClear;
    specularClear.Format = indirectSpecularFormat;
    specularClear.Color[0] = 0.0f;
    specularClear.Color[1] = 0.0f;
    specularClear.Color[2] = 0.0f;
    specularClear.Color[3] = 0.0f;

    D3D12_CLEAR_VALUE normalRoughnessClear;
    normalRoughnessClear.Format = normalRoughnessFormat;
    normalRoughnessClear.Color[0] = 1.0f;
    normalRoughnessClear.Color[1] = 1.0f;
    normalRoughnessClear.Color[2] = 1.0f;
    normalRoughnessClear.Color[3] = 1.0f;

    D3D12_CLEAR_VALUE linearDepthClear;
    linearDepthClear.Format = linearDepthFormat;
    linearDepthClear.Color[0] = 1.0f;
    linearDepthClear.Color[1] = 1.0f;
    linearDepthClear.Color[2] = 1.0f;
    linearDepthClear.Color[3] = 1.0f;

    D3D12_CLEAR_VALUE rwAccumClear;
    rwAccumClear.Format = rwAccumFormat;
    rwAccumClear.Color[0] = 0.0f;
    rwAccumClear.Color[1] = 0.0f;
    rwAccumClear.Color[2] = 0.0f;
    rwAccumClear.Color[3] = 0.0f;
  
    Texture directDiffuse = Texture(directDiffuseDesc, &directDiffuseClear,
        TextureUsage::RenderTarget,
        L"LightPass DirectDiffuse");

    Texture indirectDiffuse = Texture(indirectDiffuseDesc, &indirectDiffuseClear,
        TextureUsage::RenderTarget,
        L"LightPass IndirectDiffuse");

    Texture indirectSpecular = Texture(indirectSpecularDesc, &specularClear,
        TextureUsage::RenderTarget,
        L"LightPass IndirectSpecular");

    denoisedIndirectDiffuse = Texture(indirectDiffuseDesc, &indirectDiffuseClear,
        TextureUsage::RenderTarget,
        L"LightPass Denoised IndirectDiffuse");

    denoisedIndirectSpecular = Texture(indirectSpecularDesc, &specularClear,
        TextureUsage::RenderTarget,
        L"LightPass Denoised IndirectSpecular");

    rwAccumulation = Texture(rwAccumDesc, nullptr,
        TextureUsage::RenderTarget,
        L"LightPass AccumBuffer");

    Texture rtDebug = Texture(directDiffuseDesc, &directDiffuseClear,
        TextureUsage::RenderTarget,
        L"LightPass Raytracing debug");

    Texture normalRoughness = Texture(normalRoughnessDesc, &normalRoughnessClear,
        TextureUsage::RenderTarget,
        L"LightPass NormalRoughness");

    Texture linearDepth = Texture(linearDepthDesc, &linearDepthClear,
        TextureUsage::RenderTarget,
        L"LightPass LinearDepth");

    renderTarget.AttachTexture(AttachmentPoint::Color0, directDiffuse);
    renderTarget.AttachTexture(AttachmentPoint::Color1, indirectDiffuse);
    renderTarget.AttachTexture(AttachmentPoint::Color2, indirectSpecular);
    renderTarget.AttachTexture(AttachmentPoint::Color3, rtDebug);
    renderTarget.AttachTexture(AttachmentPoint::Color4, normalRoughness);
    renderTarget.AttachTexture(AttachmentPoint::Color5, linearDepth);

    renderTarget.AttachTexture(AttachmentPoint::Color6, denoisedIndirectDiffuse);
    renderTarget.AttachTexture(AttachmentPoint::Color7, denoisedIndirectSpecular);
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
    srvHeapRanges[1].NumDescriptors = 240;
    srvHeapRanges[1].BaseShaderRegister = 1;
    srvHeapRanges[1].RegisterSpace = 1;
    srvHeapRanges[1].OffsetInDescriptorsFromTableStart = 0;
    srvHeapRanges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    srvHeapRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvHeapRanges[2].NumDescriptors = 240;
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

    CD3DX12_DESCRIPTOR_RANGE1 gAccumBuffer = {};
    gAccumBuffer.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    gAccumBuffer.NumDescriptors = 1;
    gAccumBuffer.BaseShaderRegister = 0;
    gAccumBuffer.RegisterSpace = 0;
    gAccumBuffer.OffsetInDescriptorsFromTableStart = 0;
    gAccumBuffer.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    CD3DX12_ROOT_PARAMETER1 rootParameters[LightPassParam::Size];
    rootParameters[LightPassParam::AccelerationStructure].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);
    rootParameters[LightPassParam::GBufferHeap].InitAsDescriptorTable(1, &gBufferSRVHeap);
    rootParameters[LightPassParam::GlobalHeapData].InitAsDescriptorTable(3, srvHeapRanges);
    rootParameters[LightPassParam::GlobalMeshInfo].InitAsShaderResourceView(2, 3);
    rootParameters[LightPassParam::GlobalMatInfo].InitAsShaderResourceView(3, 4);
    rootParameters[LightPassParam::GlobalMaterials].InitAsShaderResourceView(4, 5);
    rootParameters[LightPassParam::DirectionalLightCB].InitAsConstantBufferView(0);
    rootParameters[LightPassParam::CameraCB].InitAsConstantBufferView(1);
    rootParameters[LightPassParam::RaytracingDataCB].InitAsConstantBufferView(2);
    rootParameters[LightPassParam::AccumBuffer].InitAsDescriptorTable(1, &gAccumBuffer);

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
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color3), clearColor);
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color4), clearColor);
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color5), clearColor);
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color6), clearColor);
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color7), clearColor);
}

void LightPass::OnResize(int width, int height)
{
    renderTarget.Resize(width, height);
    rwAccumulation.Resize(width, height);
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

    device->CreateShaderResourceView(
        renderTarget.GetTexture(AttachmentPoint::Color3).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(4));

    device->CreateShaderResourceView(
        renderTarget.GetTexture(AttachmentPoint::Color4).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(5));

    device->CreateShaderResourceView(
        renderTarget.GetTexture(AttachmentPoint::Color5).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(6));


    return m_SRVHeap.GetHandleAtStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE LightPass::CreateUAVViews()
{
    auto device = Application::Get().GetDevice();

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    device->CreateUnorderedAccessView(rwAccumulation.GetD3D12Resource().Get(), nullptr, &uavDesc, m_SRVHeap.SetHandle(3));

    return m_SRVHeap.GetHandleAtStart();
}