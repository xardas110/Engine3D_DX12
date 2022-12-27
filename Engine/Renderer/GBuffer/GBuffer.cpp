#include <EnginePCH.h>
#include <GBuffer/GBuffer.h>
#include <Application.h>

#include <GeometryPass_VS.h>
#include <GeometryPass_PS.h>

#include <DepthPrePass_VS.h>

#include <Application.h>

#include <DX12Helper.h>


GBuffer::GBuffer(const int& width, const int& height)
    :m_SRVHeap(20)
{
    CreateRenderTarget(width, height);
    CreatePipeline();
    CreateDepthPrePassPipeline();
}

void GBuffer::CreateRenderTarget(int width, int height)
{
    auto device = Application::Get().GetDevice();
    auto srvHeap = Application::Get().GetAssetManager()->m_SrvHeapData;

    // Create an off-screen render target with a single color buffer and a depth buffer.
    auto albedoDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
    albedoDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    albedoDesc.MipLevels = 1;

    auto normalRoughDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_UNORM, width, height);
    normalRoughDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    normalRoughDesc.MipLevels = 1;

    auto motionDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height);
    motionDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    motionDesc.MipLevels = 1;

    auto motionDesc2D = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16_FLOAT, width, height);
    motionDesc2D.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    motionDesc2D.MipLevels = 1;

    auto aoMetallicHeightDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
    aoMetallicHeightDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    aoMetallicHeightDesc.MipLevels = 1;

    auto emissiveSMDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
    emissiveSMDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    emissiveSMDesc.MipLevels = 1;
   
    auto linearDepthDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT, width, height);
    linearDepthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    linearDepthDesc.MipLevels = 1;

    auto geometryNormDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height);
    geometryNormDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    geometryNormDesc.MipLevels = 1;

    Texture albedo = Texture(albedoDesc, &ClearValue(albedoDesc.Format, {0.f, 0.f, 0.f, 0.f}),
        TextureUsage::RenderTarget,
        L"GBuffer albedo");

    Texture normalRough = Texture(normalRoughDesc, &ClearValue(normalRoughDesc.Format, { 0.f, 0.f, 0.f, 0.f }),
        TextureUsage::RenderTarget,
        L"GBuffer NormalRoughness");

    Texture motionVector = Texture(motionDesc, &ClearValue(motionDesc.Format, { 0.f, 0.f, 0.f, 0.f }),
        TextureUsage::RenderTarget,
        L"GBuffer MotionVector");

    Texture motionVector2D = Texture(motionDesc2D, &ClearValue(motionDesc2D.Format, { 0.f, 0.f, 0.f, 0.f }),
        TextureUsage::RenderTarget,
        L"GBuffer MotionVector2D");

    Texture emissive = Texture(emissiveSMDesc, &ClearValue(emissiveSMDesc.Format, { 0.f, 0.f, 0.f, 0.f }),
        TextureUsage::RenderTarget,
        L"GBuffer EmissiveShadermodel");

    Texture aoMetallicHeight = Texture(aoMetallicHeightDesc, &ClearValue(aoMetallicHeightDesc.Format, { 0.f, 0.f, 0.f, 0.f }),
        TextureUsage::RenderTarget,
        L"GBuffer AOMetallicHeight");

    Texture linearDepth = Texture(linearDepthDesc, &ClearValue(linearDepthDesc.Format, { 0.f, 0.f, 0.f, 0.f }),
        TextureUsage::RenderTarget,
        L"GBuffer LinearDepth");

    Texture geometryNormal = Texture(geometryNormDesc, &ClearValue(geometryNormDesc.Format, { 0.f, 0.f, 0.f, 0.f }),
        TextureUsage::RenderTarget,
        L"GBuffer Geometry Normal");

    // Create a depth buffer for the HDR render target.
    auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height);
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE depthClearValue;
    depthClearValue.Format = depthDesc.Format;
    depthClearValue.DepthStencil = { 1.0f, 0 };

    Texture depthTexture = Texture(depthDesc, &depthClearValue,
        TextureUsage::Depth,
        L"GBuffer depth");

    // Packing will eventually be added
    renderTarget.AttachTexture((AttachmentPoint)GBUFFER_ALBEDO, albedo);
    renderTarget.AttachTexture((AttachmentPoint)GBUFFER_NORMAL_ROUGHNESS, normalRough);
    renderTarget.AttachTexture((AttachmentPoint)GBUFFER_MOTION_VECTOR, motionVector);
    renderTarget.AttachTexture((AttachmentPoint)GBUFFER_EMISSIVE_SHADER_MODEL, emissive);
    renderTarget.AttachTexture((AttachmentPoint)GBUFFER_AO_METALLIC_HEIGHT, aoMetallicHeight);
    renderTarget.AttachTexture((AttachmentPoint)GBUFFER_LINEAR_DEPTH, linearDepth);
    renderTarget.AttachTexture((AttachmentPoint)GBUFFER_GEOMETRY_NORMAL, geometryNormal);
    renderTarget.AttachTexture((AttachmentPoint)GBUFFER_GEOMETRY_MV2D, motionVector2D);
    renderTarget.AttachTexture((AttachmentPoint)GBUFFER_STANDARD_DEPTH, depthTexture);

    CreateSRVViews();
}

void GBuffer::CreatePipeline()
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

    CD3DX12_DESCRIPTOR_RANGE1 textureTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE1 srvRanges[1] = {};
    srvRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    srvRanges[0].NumDescriptors = UINT_MAX;
    srvRanges[0].BaseShaderRegister = 1;
    srvRanges[0].RegisterSpace = 0;
    srvRanges[0].OffsetInDescriptorsFromTableStart = 0;
    srvRanges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    CD3DX12_ROOT_PARAMETER1 rootParameters[GBufferParam::Size];
    rootParameters[GBufferParam::ObjectCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[GBufferParam::CameraCB].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[GBufferParam::GlobalHeapData].InitAsDescriptorTable(1, srvRanges);
    rootParameters[GBufferParam::GlobalMatInfo].InitAsShaderResourceView(3, 4);
    rootParameters[GBufferParam::GlobalMaterials].InitAsShaderResourceView(4, 5);

    D3D12_STATIC_SAMPLER_DESC staticSampler = {};
    staticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSampler.MipLODBias = 0.f;
    staticSampler.MaxAnisotropy = 1;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSampler.MinLOD = 0.f;
    staticSampler.MaxLOD = D3D12_FLOAT32_MAX;
    staticSampler.ShaderRegister = 0;
    staticSampler.RegisterSpace = 0;
    staticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Init_1_1(GBufferParam::Size,
        rootParameters, 1, &staticSampler,
        rootSignatureFlags);

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
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormats;
       // CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DS;
       // CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
    } pipelineStateStream;

    //TODO:FIX
  //  CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
  //  rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
  //  pipelineStateStream.Rasterizer = rasterizerDesc;

   // CD3DX12_DEPTH_STENCIL_DESC dsDesc{};
   // dsDesc.DepthEnable = true;
   // dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    //dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

   // pipelineStateStream.DS = dsDesc;

    pipelineStateStream.pRootSignature = rootSignature.GetRootSignature().Get();

    pipelineStateStream.InputLayout = { VertexPositionNormalTexture::InputElements ,VertexPositionNormalTexture::InputElementCount };

    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    pipelineStateStream.VS = { g_GeometryPass_VS, sizeof(g_GeometryPass_VS) };
    pipelineStateStream.PS = { g_GeometryPass_PS, sizeof(g_GeometryPass_PS) };

    pipelineStateStream.RTVFormats = renderTarget.GetRenderTargetFormats();
    pipelineStateStream.DSVFormats = renderTarget.GetDepthStencilFormat();
    
    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipeline)));
}

void GBuffer::CreateDepthPrePassPipeline()
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

    CD3DX12_ROOT_PARAMETER1 rootParameters[DepthPrePassParam::Size];
    rootParameters[DepthPrePassParam::ObjectCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);


    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Init_1_1(DepthPrePassParam::Size,
        rootParameters, 0, nullptr,
        rootSignatureFlags);

    zPrePassRS.SetRootSignatureDesc(
        rootSignatureDesc.Desc_1_1,
        featureData.HighestVersion
    );

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
    } pipelineStateStream;

    pipelineStateStream.pRootSignature = zPrePassRS.GetRootSignature().Get();

    pipelineStateStream.InputLayout = { VertexPositionNormalTexture::InputElements ,VertexPositionNormalTexture::InputElementCount };

    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    pipelineStateStream.VS = { g_DepthPrePass_VS, sizeof(g_DepthPrePass_VS) };
    pipelineStateStream.DSVFormat = renderTarget.GetDepthStencilFormat();

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&zPrePassPipeline)));
}

void GBuffer::ClearRendetTarget(CommandList& commandlist, float clearColor[4])
{
    float zeroColor[] = { 0.f, 0.f, 0.f, 0.f };
    float skyClear[] = { 50001.f, 50001.f, 50001.f, 50001.f };

    commandlist.ClearTexture(renderTarget.GetTexture((AttachmentPoint)GBUFFER_ALBEDO), zeroColor);

    commandlist.ClearTexture(renderTarget.GetTexture((AttachmentPoint)GBUFFER_NORMAL_ROUGHNESS), zeroColor);

    commandlist.ClearTexture(renderTarget.GetTexture((AttachmentPoint)GBUFFER_MOTION_VECTOR), zeroColor);

    commandlist.ClearTexture(renderTarget.GetTexture((AttachmentPoint)GBUFFER_EMISSIVE_SHADER_MODEL), zeroColor);

    commandlist.ClearTexture(renderTarget.GetTexture((AttachmentPoint)GBUFFER_AO_METALLIC_HEIGHT), zeroColor);

    commandlist.ClearTexture(renderTarget.GetTexture((AttachmentPoint)GBUFFER_LINEAR_DEPTH), skyClear);

    commandlist.ClearTexture(renderTarget.GetTexture((AttachmentPoint)GBUFFER_GEOMETRY_NORMAL), zeroColor);

    commandlist.ClearTexture(renderTarget.GetTexture((AttachmentPoint)GBUFFER_GEOMETRY_MV2D), zeroColor);

    commandlist.ClearDepthStencilTexture(renderTarget.GetTexture((AttachmentPoint)GBUFFER_STANDARD_DEPTH), D3D12_CLEAR_FLAG_DEPTH);
}

void GBuffer::OnResize(int width, int height)
{
    renderTarget.Resize(width, height);
    CreateSRVViews();
}

D3D12_GPU_DESCRIPTOR_HANDLE GBuffer::CreateSRVViews()
{
    auto device = Application::Get().GetDevice();

    device->CreateShaderResourceView(
        GetTexture(GBUFFER_ALBEDO).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(GBUFFER_ALBEDO));

    device->CreateShaderResourceView(
        GetTexture(GBUFFER_NORMAL_ROUGHNESS).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(GBUFFER_NORMAL_ROUGHNESS));

    device->CreateShaderResourceView(
        GetTexture(GBUFFER_MOTION_VECTOR).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(GBUFFER_MOTION_VECTOR));

    device->CreateShaderResourceView(
        GetTexture(GBUFFER_EMISSIVE_SHADER_MODEL).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(GBUFFER_EMISSIVE_SHADER_MODEL));

    device->CreateShaderResourceView(
        GetTexture(GBUFFER_AO_METALLIC_HEIGHT).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(GBUFFER_AO_METALLIC_HEIGHT));

    device->CreateShaderResourceView(
        GetTexture(GBUFFER_LINEAR_DEPTH).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(GBUFFER_LINEAR_DEPTH));

    device->CreateShaderResourceView(
        GetTexture(GBUFFER_GEOMETRY_NORMAL).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(GBUFFER_GEOMETRY_NORMAL));

    device->CreateShaderResourceView(
        GetTexture(GBUFFER_GEOMETRY_MV2D).GetD3D12Resource().Get(), nullptr,
        m_SRVHeap.SetHandle(GBUFFER_GEOMETRY_MV2D));

    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.Format = DXGI_FORMAT_R32_FLOAT;
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Texture2D.MipLevels = 1;
    desc.Texture2D.MostDetailedMip = 0;

    device->CreateShaderResourceView(
        GetTexture(GBUFFER_STANDARD_DEPTH).GetD3D12Resource().Get(), &desc,
        m_SRVHeap.SetHandle(GBUFFER_STANDARD_DEPTH));

    return m_SRVHeap.GetHandleAtStart();
}

const Texture& GBuffer::GetTexture(int type)
{
  return renderTarget.GetTexture((AttachmentPoint)type);
}
