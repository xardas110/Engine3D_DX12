#include "EnginePCH.h"
#include "DeferredRenderer.h"
#include <Game.h>
#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <Window.h>
#include <Transform.h>
#include <AssimpLoader.h>

using namespace DirectX;

struct Mat
{
    DirectX::XMMATRIX model; // Updates pr. object
    DirectX::XMMATRIX mvp; // Updates pr. object
    DirectX::XMMATRIX invTransposeMvp; // Updates pr. object

    DirectX::XMMATRIX view; // Updates pr. frame
    DirectX::XMMATRIX proj; // Updates pr. frame

    DirectX::XMMATRIX invView; // Updates pr. frame
    DirectX::XMMATRIX invProj; // Updates pr. frame
};


DeferredRenderer::DeferredRenderer(int width, int height)
    :m_Width(width), m_Height(height)
{
	CreateGBuffer();

    auto currentDir = std::filesystem::current_path();

    std::cout << currentDir << std::endl;

    AssimpLoader loader("Assets/Models/crytek-sponza-noflag/sponza.dae");
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

void DeferredRenderer::Render(Window& window)
{
    if (!window.m_pGame.lock()) return;
    
    auto game = window.m_pGame.lock();

    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();

    auto& pipelines = Application::Get().GetPipelineManager()->m_Pipelines;
    auto& primitves = Application::Get().GetAssetManager()->m_Primitives;

    auto* camera = game->GetRenderCamera();

    Mat mat;
    mat.view = camera->get_ViewMatrix();
    mat.proj = camera->get_ProjectionMatrix();
    mat.invView = XMMatrixInverse(nullptr, mat.view);
    mat.invProj = XMMatrixInverse(nullptr, mat.proj);

    // Clear the render targets.
    {
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

        commandList->ClearTexture(m_GBufferRenderTarget.GetTexture(AttachmentPoint::Color0), clearColor);
        commandList->ClearDepthStencilTexture(m_GBufferRenderTarget.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
    }

    commandList->SetRenderTarget(m_GBufferRenderTarget);
    commandList->SetViewport(m_GBufferRenderTarget.GetViewport());
    commandList->SetScissorRect(m_ScissorRect);
    commandList->SetPipelineState(pipelines[Pipeline::GeometryMesh].pipelineRef);
    commandList->SetGraphicsRootSignature(pipelines[Pipeline::GeometryMesh].rootSignature);

    Transform transform;
    transform.pos = { 0.f, 5.f, -10.f, 0.f };
    transform.rot = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(45.f), XMConvertToRadians(45.f), XMConvertToRadians(45.f));
    mat.model = transform.GetTransform();
    mat.mvp = mat.model * mat.view * mat.proj;
    mat.invTransposeMvp = XMMatrixInverse(nullptr, XMMatrixTranspose(mat.mvp));

    commandList->SetGraphicsDynamicConstantBuffer(GeometryMeshRootParam::MatCB, mat);
    
    primitves[Primitives::Cube].Draw(*commandList);

    commandQueue->ExecuteCommandList(commandList);
}

void DeferredRenderer::OnResize(ResizeEventArgs& e)
{
    m_Width = std::max(1, e.Width);
    m_Height = std::max(1, e.Height);

    m_GBufferRenderTarget.Resize(m_Width, m_Height);
}
