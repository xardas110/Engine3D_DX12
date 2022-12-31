#include <EnginePCH.h>
#include <Lightpass/LightPass.h>
#include <Application.h>

#include <LightPass_VS.h>
#include <LightPass_PS.h>

#include <DX12Helper.h>
#include <TypesCompat.h>

LightPass::LightPass(const int& width, const int& height)
{
    CreateRenderTarget(width, height);
    CreatePipeline();
}

void LightPass::CreateRenderTarget(int width, int height)
{
    auto directDiffuseDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height);
    directDiffuseDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    directDiffuseDesc.MipLevels = 1;

    auto shadowDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16_FLOAT, width, height);
    shadowDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    shadowDesc.MipLevels = 1;

    auto outShadowDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16_UNORM, width, height);
    outShadowDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    outShadowDesc.MipLevels = 1;

    auto transparentColorDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height);
    transparentColorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    transparentColorDesc.MipLevels = 1;

    auto indirectDiffuseDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height);
    indirectDiffuseDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    indirectDiffuseDesc.MipLevels = 1;

    auto indirectSpecularDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height);
    indirectSpecularDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    indirectSpecularDesc.MipLevels = 1;

    auto outIndirectDiffuseDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height);
    outIndirectDiffuseDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    outIndirectDiffuseDesc.MipLevels = 1;

    auto outIndirectSpecularDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height);
    outIndirectSpecularDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    outIndirectSpecularDesc.MipLevels = 1;

    auto rtDebugDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM , width, height);
    rtDebugDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    rtDebugDesc.MipLevels = 1;

    Texture directDiffuse = Texture(directDiffuseDesc, &ClearValue(directDiffuseDesc.Format, {0.f, 0.f, 0.f, 0.f}),
        TextureUsage::RenderTarget,
        L"LightPass DirectDiffuse");

    Texture shadow = Texture(shadowDesc, &ClearValue(shadowDesc.Format, { 0.f, 0.f, 0.f, 0.f }),
        TextureUsage::RenderTarget,
        L"LightPass shadow");

    denoisedShadow = Texture(outShadowDesc, nullptr,
        TextureUsage::RenderTarget,
        L"LightPass denoised shadow");

    Texture transparentColor = Texture(transparentColorDesc, &ClearValue(transparentColorDesc.Format, { 0.f, 0.f, 0.f, 0.f }),
        TextureUsage::RenderTarget,
        L"LightPass TransparentColor");

    Texture indirectDiffuse = Texture(indirectDiffuseDesc, &ClearValue(indirectDiffuseDesc.Format, { 0.f, 0.f, 0.f, 0.f }),
        TextureUsage::RenderTarget,
        L"LightPass IndirectDiffuse");

    Texture indirectSpecular = Texture(indirectSpecularDesc, &ClearValue(indirectSpecularDesc.Format, { 0.f, 0.f, 0.f, 0.f }),
        TextureUsage::RenderTarget,
        L"LightPass IndirectSpecular");

    Texture rtDebug = Texture(rtDebugDesc, &ClearValue(rtDebugDesc.Format, { 0.f, 0.f, 0.f, 0.f }),
        TextureUsage::RenderTarget,
        L"LightPass RtDebug");

    denoisedIndirectDiffuse = Texture(outIndirectDiffuseDesc, nullptr,
        TextureUsage::RenderTarget,
        L"LightPass Denoised IndirectDiffuse");

    denoisedIndirectSpecular = Texture(outIndirectSpecularDesc, nullptr,
        TextureUsage::RenderTarget,
        L"LightPass Denoised IndirectSpecular");

    renderTarget.AttachTexture((AttachmentPoint)LIGHTBUFFER_DIRECT_LIGHT, directDiffuse);
    renderTarget.AttachTexture((AttachmentPoint)LIGHTBUFFER_INDIRECT_DIFFUSE, indirectDiffuse);
    renderTarget.AttachTexture((AttachmentPoint)LIGHTBUFFER_INDIRECT_SPECULAR, indirectSpecular);   
    renderTarget.AttachTexture((AttachmentPoint)LIGHTBUFFER_RT_DEBUG, rtDebug);
    renderTarget.AttachTexture(AttachmentPoint::Color4, transparentColor);
    renderTarget.AttachTexture((AttachmentPoint)Color5, shadow);

    CreateSRVViews();
    CreateUAVViews();
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
    gBufferSRVHeap.NumDescriptors = GBUFFER_NUM;
    gBufferSRVHeap.BaseShaderRegister = 5;
    gBufferSRVHeap.RegisterSpace = 6;
    gBufferSRVHeap.OffsetInDescriptorsFromTableStart = 0;
   // gBufferSRVHeap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

    CD3DX12_DESCRIPTOR_RANGE1 gAccumBuffer = {};
    gAccumBuffer.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    gAccumBuffer.NumDescriptors = 1;
    gAccumBuffer.BaseShaderRegister = 0;
    gAccumBuffer.RegisterSpace = 0;
    gAccumBuffer.OffsetInDescriptorsFromTableStart = 0;
    gAccumBuffer.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    CD3DX12_DESCRIPTOR_RANGE1 cubemapSRVHeap = {};
    cubemapSRVHeap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    cubemapSRVHeap.NumDescriptors = 6;
    cubemapSRVHeap.BaseShaderRegister = 7;
    cubemapSRVHeap.RegisterSpace = 8;
    cubemapSRVHeap.OffsetInDescriptorsFromTableStart = 0;
    cubemapSRVHeap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    CD3DX12_DESCRIPTOR_RANGE1 denoiserHeap = {};
    denoiserHeap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    denoiserHeap.NumDescriptors = DENOISER_TEX_NUM;
    denoiserHeap.BaseShaderRegister = 8;
    denoiserHeap.RegisterSpace = 9;
    denoiserHeap.OffsetInDescriptorsFromTableStart = 0;
    denoiserHeap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;


    CD3DX12_ROOT_PARAMETER1 rootParameters[LightPassParam::Size];
    rootParameters[LightPassParam::AccelerationStructure].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);
    rootParameters[LightPassParam::GBufferHeap].InitAsDescriptorTable(1, &gBufferSRVHeap, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[LightPassParam::GlobalHeapData].InitAsDescriptorTable(3, srvHeapRanges);
    rootParameters[LightPassParam::GlobalMeshInfo].InitAsShaderResourceView(2, 3);
    rootParameters[LightPassParam::GlobalMatInfo].InitAsShaderResourceView(3, 4);
    rootParameters[LightPassParam::GlobalMaterials].InitAsShaderResourceView(4, 5);
    rootParameters[LightPassParam::DirectionalLightCB].InitAsConstantBufferView(0);
    rootParameters[LightPassParam::CameraCB].InitAsConstantBufferView(1);
    rootParameters[LightPassParam::RaytracingDataCB].InitAsConstantBufferView(2);
    rootParameters[LightPassParam::AccumBuffer].InitAsDescriptorTable(1, &gAccumBuffer);
    rootParameters[LightPassParam::Cubemap].InitAsDescriptorTable(1, &cubemapSRVHeap);
    rootParameters[LightPassParam::BlueNoiseTextures].InitAsDescriptorTable(1, &denoiserHeap);

    D3D12_STATIC_SAMPLER_DESC staticSampler[3] = {};
    staticSampler[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler[0].MipLODBias = 0.f;
    staticSampler[0].MaxAnisotropy = 1;
    staticSampler[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    staticSampler[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSampler[0].MinLOD = 0.f;
    staticSampler[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler[0].ShaderRegister = 0;
    staticSampler[0].RegisterSpace = 0;
    staticSampler[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    staticSampler[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler[1].MipLODBias = 0.f;
    staticSampler[1].MaxAnisotropy = 1;
    staticSampler[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    staticSampler[1].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSampler[1].MinLOD = 0.f;
    staticSampler[1].MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler[1].ShaderRegister = 1;
    staticSampler[1].RegisterSpace = 0;
    staticSampler[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    staticSampler[2].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    staticSampler[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    staticSampler[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    staticSampler[2].MipLODBias = 0.f;
    staticSampler[2].MaxAnisotropy = 1;
    staticSampler[2].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    staticSampler[2].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSampler[2].MinLOD = 0.f;
    staticSampler[2].MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler[2].ShaderRegister = 2;
    staticSampler[2].RegisterSpace = 0;
    staticSampler[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Init_1_1(LightPassParam::Size,
        rootParameters, 3, staticSampler);

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
    float clearValue[] = { 0.f, 0.f, 0.f, 0.f };
    float clearValueShadow[] = { 0.f, 0.f, 0.f, 0.f };

    commandlist.ClearTexture(GetTexture(LIGHTBUFFER_DIRECT_LIGHT), clearValue);
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color4), clearValue);
    commandlist.ClearTexture(GetTexture(LIGHTBUFFER_INDIRECT_DIFFUSE), clearValue);
    commandlist.ClearTexture(GetTexture(LIGHTBUFFER_INDIRECT_SPECULAR), clearValue);
    commandlist.ClearTexture(GetTexture(LIGHTBUFFER_SHADOW_DATA), clearValue);
    commandlist.ClearTexture(GetTexture(LIGHTBUFFER_RT_DEBUG), clearColor);
}

void LightPass::OnResize(int width, int height)
{
    renderTarget.Resize(width, height);
    denoisedIndirectDiffuse.Resize(width, height);
    denoisedIndirectSpecular.Resize(width, height);
    denoisedShadow.Resize(width, height);

    CreateSRVViews();
    CreateUAVViews();
}

D3D12_GPU_DESCRIPTOR_HANDLE LightPass::CreateSRVViews()
{
    auto device = Application::Get().GetDevice();

    device->CreateShaderResourceView(
        GetTexture(LIGHTBUFFER_DIRECT_LIGHT).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(LIGHTBUFFER_DIRECT_LIGHT));

    device->CreateShaderResourceView(
        GetTexture(LIGHTBUFFER_INDIRECT_DIFFUSE).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(LIGHTBUFFER_INDIRECT_DIFFUSE));

    device->CreateShaderResourceView(
        GetTexture(LIGHTBUFFER_INDIRECT_SPECULAR).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(LIGHTBUFFER_INDIRECT_SPECULAR));

    device->CreateShaderResourceView(
        GetTexture(LIGHTBUFFER_RT_DEBUG).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(LIGHTBUFFER_RT_DEBUG));

    device->CreateShaderResourceView(
        GetTexture(LIGHTBUFFER_DENOISED_INDIRECT_DIFFUSE).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(LIGHTBUFFER_DENOISED_INDIRECT_DIFFUSE));

    device->CreateShaderResourceView(
        GetTexture(LIGHTBUFFER_DENOISED_INDIRECT_SPECULAR).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(LIGHTBUFFER_DENOISED_INDIRECT_SPECULAR));

    device->CreateShaderResourceView(
        GetTexture(LIGHTBUFFER_SHADOW_DATA).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(LIGHTBUFFER_SHADOW_DATA));

    device->CreateShaderResourceView(
        GetTexture(LIGHTBUFFER_DENOISED_SHADOW).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(LIGHTBUFFER_DENOISED_SHADOW));

    device->CreateShaderResourceView(
        renderTarget.GetTexture(AttachmentPoint::Color4).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(LIGHTBUFFER_TRANSPARENT_COLOR));

    return m_SRVHeap.GetHandleAtStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE LightPass::CreateUAVViews()
{
    auto device = Application::Get().GetDevice();

    /*
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

    device->CreateUnorderedAccessView(GetTexture(LIGHTBUFFER_ACCUM_BUFFER).GetD3D12Resource().Get(), nullptr, &uavDesc, m_SRVHeap.SetHandle(LIGHTBUFFER_ACCUM_BUFFER));
    */

    return m_SRVHeap.GetHandleAtStart();
}

const Texture& LightPass::GetTexture(int type)
{
    if (type == LIGHTBUFFER_DENOISED_INDIRECT_DIFFUSE) return denoisedIndirectDiffuse;
    if (type == LIGHTBUFFER_DENOISED_INDIRECT_SPECULAR) return denoisedIndirectSpecular;
    if (type == LIGHTBUFFER_DENOISED_SHADOW) return denoisedShadow;
    if (type == LIGHTBUFFER_SHADOW_DATA) return renderTarget.GetTexture((AttachmentPoint)Color5);

    return renderTarget.GetTexture((AttachmentPoint)type);
}
