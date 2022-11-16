#include <EnginePCH.h>
#include <GBuffer/GBuffer.h>
#include <Application.h>

#include <GeometryPass_VS.h>
#include <GeometryPass_PS.h>

#include <DepthPrePass_VS.h>

GBuffer::GBuffer(const int& width, const int& height)
{
    CreateRenderTarget(width, height);
    CreatePipeline();
    CreateDepthPrePassPipeline();
}

void GBuffer::CreateRenderTarget(int width, int height)
{
    // Create an HDR intermediate render target.
    DXGI_FORMAT albedoFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT normalFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
    DXGI_FORMAT pbrFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

    // Create an off-screen render target with a single color buffer and a depth buffer.
    auto albedoDesc = CD3DX12_RESOURCE_DESC::Tex2D(albedoFormat, width, height);
    albedoDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    albedoDesc.MipLevels = 1;

    auto normalDesc = CD3DX12_RESOURCE_DESC::Tex2D(normalFormat, width, height);
    normalDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    normalDesc.MipLevels = 1;

    auto pbrDesc = CD3DX12_RESOURCE_DESC::Tex2D(pbrFormat, width, height);
    pbrDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    pbrDesc.MipLevels = 1;

    D3D12_CLEAR_VALUE albedoClearValue;
    albedoClearValue.Format = albedoDesc.Format;
    albedoClearValue.Color[0] = 1.0f;
    albedoClearValue.Color[1] = 1.0f;
    albedoClearValue.Color[2] = 1.0f;
    albedoClearValue.Color[3] = 1.0f;

    D3D12_CLEAR_VALUE normalPBRClearValue;
    normalPBRClearValue.Format = normalDesc.Format;
    normalPBRClearValue.Color[0] = 1.0f;
    normalPBRClearValue.Color[1] = 1.0f;
    normalPBRClearValue.Color[2] = 1.0f;
    normalPBRClearValue.Color[3] = 1.0f;

    Texture albedo = Texture(albedoDesc, &albedoClearValue,
        TextureUsage::RenderTarget,
        L"GBuffer albedo");

    Texture normal = Texture(normalDesc, &normalPBRClearValue,
        TextureUsage::RenderTarget,
        L"GBuffer normal");

    Texture pbr = Texture(pbrDesc, &normalPBRClearValue,
        TextureUsage::RenderTarget,
        L"GBuffer PBR");

    // Create a depth buffer for the HDR render target.
    auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, width, height);
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE depthClearValue;
    depthClearValue.Format = depthDesc.Format;
    depthClearValue.DepthStencil = { 1.0f, 0 };

    Texture depthTexture = Texture(depthDesc, &depthClearValue,
        TextureUsage::Depth,
        L"GBuffer depth");

    // Attach the HDR texture to the HDR render target.
    renderTarget.AttachTexture(AttachmentPoint::Color0, albedo);
    renderTarget.AttachTexture(AttachmentPoint::Color1, normal);
    renderTarget.AttachTexture(AttachmentPoint::Color2, pbr);
    renderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);
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

    CD3DX12_ROOT_PARAMETER1 rootParameters[GBufferParam::Size];
    rootParameters[GBufferParam::ObjectCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[GBufferParam::Textures].InitAsDescriptorTable(1, &textureTable, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Init_1_1(GBufferParam::Size,
        rootParameters, 1, &linearRepeatSampler,
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
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DS;
    } pipelineStateStream;

    CD3DX12_DEPTH_STENCIL_DESC dsDesc{};
    dsDesc.DepthEnable = true;
    dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    
    pipelineStateStream.DS = dsDesc;

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
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color0), clearColor);
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color1), clearColor);
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color2), clearColor);
    commandlist.ClearDepthStencilTexture(renderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
}

void GBuffer::OnResize(int width, int height)
{
    renderTarget.Resize(width, height);
}
