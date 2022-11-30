#include <EnginePCH.h>
#include <../Debug/DebugTexture.h>
#include <DebugTexture_VS.h>
#include <DebugTexture_PS.h>
#include <Application.h>

void DebugTexturePass::CreatePipeline()
{
    auto device = Application::Get().GetDevice();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE1 textureHeap = {};
    textureHeap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    textureHeap.NumDescriptors = 1;
    textureHeap.BaseShaderRegister = 0;
    textureHeap.RegisterSpace = 0;
    textureHeap.OffsetInDescriptorsFromTableStart = 0;

    CD3DX12_ROOT_PARAMETER1 rootParameters[DebugTextureParam::Size];
    rootParameters[DebugTextureParam::texture].InitAsDescriptorTable(1, &textureHeap, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC samplers[1];
    samplers[0] = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_COMPARISON_ANISOTROPIC);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Init_1_1(DebugTextureParam::Size,
        rootParameters, 1, samplers);

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

    pipelineStateStream.VS = { g_DebugTexture_VS, sizeof(g_DebugTexture_VS) };
    pipelineStateStream.PS = { g_DebugTexture_PS, sizeof(g_DebugTexture_PS) };

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    pipelineStateStream.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipeline)));
}

void DebugTexturePass::CreateRenderTarget(int width, int height)
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
        L"Debug buffer");

    renderTarget.AttachTexture(AttachmentPoint::Color0, color);
}

void DebugTexturePass::ClearRendetTarget(CommandList& commandlist, float clearColor[4])
{
    commandlist.ClearTexture(renderTarget.GetTexture(AttachmentPoint::Color0), clearColor);
}

void DebugTexturePass::OnResize(int width, int height)
{
    renderTarget.Resize(width, height);
}
