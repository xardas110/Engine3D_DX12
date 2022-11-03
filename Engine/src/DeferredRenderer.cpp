#include "EnginePCH.h"
#include "DeferredRenderer.h"
#include <Game.h>
#include <Application.h>

DeferredRenderer::DeferredRenderer(int width, int height)
    :m_Width(width), m_Height(height)
{
	CreateGBuffer();
}

DeferredRenderer::~DeferredRenderer()
{
}

void DeferredRenderer::CreateGBuffer()
{
    // Create an HDR intermediate render target.
    DXGI_FORMAT b0 = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

    // Create an off-screen render target with a single color buffer and a depth buffer.
    auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(b0, m_Width, m_Height);
    colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    colorDesc.MipLevels = 1;

    D3D12_CLEAR_VALUE colorClearValue;
    colorClearValue.Format = colorDesc.Format;
    colorClearValue.Color[0] = 1.0f;
    colorClearValue.Color[1] = 1.0f;
    colorClearValue.Color[2] = 1.0f;
    colorClearValue.Color[3] = 1.0f;

    Texture b0Texture = Texture(colorDesc, &colorClearValue,
        TextureUsage::RenderTarget,
        L"GBuffer b0");

    // Create a depth buffer for the HDR render target.
    auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, m_Width, m_Height);
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE depthClearValue;
    depthClearValue.Format = depthDesc.Format;
    depthClearValue.DepthStencil = { 1.0f, 0 };

    Texture depthTexture = Texture(depthDesc, &depthClearValue,
        TextureUsage::Depth,
        L"GBuffer depth");

    // Attach the HDR texture to the HDR render target.
    m_GBufferRenderTarget.AttachTexture(AttachmentPoint::Color0, b0Texture);
    m_GBufferRenderTarget.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);
}

void DeferredRenderer::Render(std::shared_ptr<Game>& game)
{
}

void DeferredRenderer::OnResize(ResizeEventArgs& e)
{
    m_Width = e.Width; m_Height = e.Height;
    m_GBufferRenderTarget.Resize(m_Width, m_Height);
}
