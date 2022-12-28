#include <EnginePCH.h>
#include "DirectLightPass.h"

#include <Application.h>

#include <DX12Helper.h>
#include <TypesCompat.h>

#include <DirectLightPass_VS.h>
#include <DirectLightPass_PS.h>

#include <DirectLightShadowPass_VS.h>
#include <DirectLightShadowPass_PS.h>

DirectLightPass::DirectLightPass(const int& width, const int& height)
{
    CreateRenderTargets(width, height);
    CreateDirectLightPipeline();
    CreateDirectLightShadowPipeline();
    CreateSRVViews();
}

void DirectLightPass::CreateRenderTargets(int width, int height)
{
    auto directLightDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16B16A16_FLOAT, width, height);
    directLightDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    directLightDesc.MipLevels = 1;

    Texture directLight = Texture(directLightDesc, &ClearValue(directLightDesc.Format, { 0.f, 0.f, 0.f, 1.f }),
        TextureUsage::RenderTarget,
        L"DirectLight");

    auto directLightShadowDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UNORM, width, height);
    directLightShadowDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    directLightShadowDesc.MipLevels = 1;

    Texture directLightShadow = Texture(directLightShadowDesc, &ClearValue(directLightShadowDesc.Format, { 1.f, 1.f, 1.f, 1.f }),
        TextureUsage::RenderTarget,
        L"DirectLightShadow");

    directLightRT.AttachTexture((AttachmentPoint)0, directLight);
    directLightShadowRT.AttachTexture((AttachmentPoint)0, directLightShadow);
}

void DirectLightPass::CreateDirectLightPipeline()
{
    auto device = Application::Get().GetDevice();

    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    CD3DX12_DESCRIPTOR_RANGE1 gBufferSRVHeap = {};
    gBufferSRVHeap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    gBufferSRVHeap.NumDescriptors = GBUFFER_NUM;
    gBufferSRVHeap.BaseShaderRegister = 5;
    gBufferSRVHeap.RegisterSpace = 6;
    gBufferSRVHeap.OffsetInDescriptorsFromTableStart = 0;
    // gBufferSRVHeap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

    CD3DX12_DESCRIPTOR_RANGE1 cubemapSRVHeap = {};
    cubemapSRVHeap.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    cubemapSRVHeap.NumDescriptors = 6;
    cubemapSRVHeap.BaseShaderRegister = 7;
    cubemapSRVHeap.RegisterSpace = 8;
    cubemapSRVHeap.OffsetInDescriptorsFromTableStart = 0;
    cubemapSRVHeap.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;

    CD3DX12_ROOT_PARAMETER1 rootParameters[DirectLightPassParam::Size];
    rootParameters[DirectLightPassParam::GBufferHeap].InitAsDescriptorTable(1, &gBufferSRVHeap, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[DirectLightPassParam::DirectionalLightCB].InitAsConstantBufferView(0);
    rootParameters[DirectLightPassParam::CameraCB].InitAsConstantBufferView(1);
    rootParameters[DirectLightPassParam::Cubemap].InitAsDescriptorTable(1, &cubemapSRVHeap);

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
    rootSignatureDesc.Init_1_1(DirectLightPassParam::Size,
        rootParameters, 3, staticSampler);

    directLightRS.SetRootSignatureDesc(
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

    pipelineStateStream.pRootSignature = directLightRS.GetRootSignature().Get();

    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    pipelineStateStream.VS = { g_DirectLightPass_VS, sizeof(g_DirectLightPass_VS) };
    pipelineStateStream.PS = { g_DirectLightPass_PS, sizeof(g_DirectLightPass_PS) };

    pipelineStateStream.RTVFormats = directLightRT.GetRenderTargetFormats();

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&directLightPSO)));
}

void DirectLightPass::CreateDirectLightShadowPipeline()
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

    CD3DX12_ROOT_PARAMETER1 rootParameters[DirectLightShadowPassParam::Size];
    rootParameters[DirectLightShadowPassParam::AccelerationStructure].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE);
    rootParameters[DirectLightShadowPassParam::GBufferHeap].InitAsDescriptorTable(1, &gBufferSRVHeap, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[DirectLightShadowPassParam::GlobalHeapData].InitAsDescriptorTable(3, srvHeapRanges);
    rootParameters[DirectLightShadowPassParam::GlobalMeshInfo].InitAsShaderResourceView(2, 3);
    rootParameters[DirectLightShadowPassParam::GlobalMatInfo].InitAsShaderResourceView(3, 4);
    rootParameters[DirectLightShadowPassParam::GlobalMaterials].InitAsShaderResourceView(4, 5);
    rootParameters[DirectLightShadowPassParam::DirectionalLightCB].InitAsConstantBufferView(0);
    rootParameters[DirectLightShadowPassParam::CameraCB].InitAsConstantBufferView(1);
    rootParameters[DirectLightShadowPassParam::RaytracingDataCB].InitAsConstantBufferView(2);

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
    rootSignatureDesc.Init_1_1(DirectLightShadowPassParam::Size,
        rootParameters, 3, staticSampler);

    directLightShadowRS.SetRootSignatureDesc(
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

    pipelineStateStream.pRootSignature = directLightShadowRS.GetRootSignature().Get();

    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    pipelineStateStream.VS = { g_DirectLightShadowPass_VS, sizeof(g_DirectLightShadowPass_VS) };
    pipelineStateStream.PS = { g_DirectLightShadowPass_PS, sizeof(g_DirectLightShadowPass_PS) };

    pipelineStateStream.RTVFormats = directLightShadowRT.GetRenderTargetFormats();

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream
    };

    ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&directLightShadowPSO)));

}

void DirectLightPass::ClearRendetTargets(CommandList& commandlist, float clearColor[4])
{
    float clearValue1[] = { 0.f, 0.f, 0.f, 1.f };
    float clearValue2[] = { 1.f, 1.f, 1.f, 1.f };

    commandlist.ClearTexture(directLightRT.GetTexture(AttachmentPoint::Color0), clearValue1);
    commandlist.ClearTexture(directLightShadowRT.GetTexture(AttachmentPoint::Color0), clearValue2);
}

void DirectLightPass::OnResize(int width, int height)
{
    directLightRT.Resize(width, height);
    directLightShadowRT.Resize(width, height);
    CreateSRVViews();
}

void DirectLightPass::CreateSRVViews()
{
    auto device = Application::Get().GetDevice();

    device->CreateShaderResourceView(
        directLightRT.GetTexture(AttachmentPoint::Color0).GetD3D12Resource().Get(), nullptr,
        directLightHeap.SetHandle(0));

    device->CreateShaderResourceView(
        directLightShadowRT.GetTexture(AttachmentPoint::Color0).GetD3D12Resource().Get(), nullptr,
        shadowHeap.SetHandle(0));
}
