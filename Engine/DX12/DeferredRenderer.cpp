#include "EnginePCH.h"
#include "DeferredRenderer.h"
#include <Game.h>
#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <Window.h>
#include <Transform.h>
#include <AssimpLoader.h>
#include <Components.h>
#include <entt/entt.hpp>
#include <Entity.h>
#include <Helpers.h>
#include <Entity.h>
#include <Components.h>
#include <RayTracingHlslCompat.h>

bool IsDirectXRaytracingSupported(IDXGIAdapter4* adapter)
{
    ComPtr<ID3D12Device> testDevice;
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};
    featureSupportData.RaytracingTier = D3D12_RAYTRACING_TIER_1_1;

    return SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))
        && SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
        && featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}

using namespace DirectX;

DeferredRenderer::DeferredRenderer(int width, int height)
    :m_Width(width), m_Height(height)
{
    //auto adapter = Application::Get().GetAdapter(false);  
    //assert(IsDirectXRaytracingSupported(adapter.Get()));
    std::cout << "DeferredRenderer: Raytracing is supported on the device" << std::endl;

    m_Raytracer = std::unique_ptr<Raytracing>(new Raytracing);
    m_Raytracer->Init();

	CreateGBuffer();

     

    /*
 StaticMesh temp;

 auto* smm = Application::Get().GetAssetManager();
 smm->LoadStaticMesh("Assets/Models/crytek-sponza-noflag/sponza.dae", temp);
 */

/*
    auto ent = CreateEntity("Sponza");
    auto& sm = ent.AddComponent<StaticMeshComponent>("Assets/Models/crytek-sponza-noflag/sponza.dae");
    auto& trans = ent.GetComponent<TransformComponent>().scale = { 0.01f, 0.01f, 0.01f };
*/
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
    auto& meshes = Application::Get().GetAssetManager()->m_Meshes;
    auto& textureHeap = Application::Get().GetAssetManager()->m_TextureData.heap;

    auto* camera = game->GetRenderCamera();

    ObjectCB objectCB;
    objectCB.view = camera->get_ViewMatrix();
    objectCB.proj = camera->get_ProjectionMatrix();
    objectCB.invView = XMMatrixInverse(nullptr, objectCB.view);
    objectCB.invProj = XMMatrixInverse(nullptr, objectCB.proj);

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
    commandList->GetGraphicsCommandList()->SetGraphicsRootShaderResourceView(
        GeometryMeshRootParam::AccelerationStructure, 
        m_Raytracer->m_RaytracingAccelerationStructure.topLevelAccelerationStructure->GetGPUVirtualAddress());
  

    /*
    Transform f;
    f.pos = { 0.f, -20.f, 0.f };
    f.scale = { 10.f, 10.f, 10.f };
    mat.model = f.GetTransform();
    mat.mvp = mat.model * mat.view * mat.proj;
    mat.invTransposeMvp = XMMatrixInverse(nullptr, XMMatrixTranspose(mat.mvp));   
    commandList->SetGraphicsDynamicConstantBuffer(GeometryMeshRootParam::MatCB, mat);
    m_Raytracer->m_Cube->Draw(*commandList);

    f.pos = { 1.f, -5.f, 0.f };
    f.scale = { 1.f, 1.f, 1.f };
    mat.model = f.GetTransform();
    mat.mvp = mat.model * mat.view * mat.proj;
    mat.invTransposeMvp = XMMatrixInverse(nullptr, XMMatrixTranspose(mat.mvp));
    commandList->SetGraphicsDynamicConstantBuffer(GeometryMeshRootParam::MatCB, mat);
    */
    /*
    commandList->SetShaderResourceView(
        GeometryMeshRootParam::Textures, 0,
        m_DXTexture,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        */
    //m_Raytracer->m_Cube->Draw(*commandList);

    commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, textureHeap.Get());
    commandList->GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(GeometryMeshRootParam::Textures, textureHeap->GetGPUDescriptorHandleForHeapStart());

    auto& view = game->registry.view<TransformComponent, MeshComponent, TextureComponent>();
    for (auto [entity, transform, mesh, texture] : view.each())
    {
        objectCB.model = transform.GetTransform();
        objectCB.mvp = objectCB.model * objectCB.view * objectCB.proj;
        objectCB.invTransposeMvp = XMMatrixInverse(nullptr, XMMatrixTranspose(objectCB.mvp));
        objectCB.entId = (UINT)entity;
        objectCB.textureId = texture.textureID;

        commandList->SetGraphicsDynamicConstantBuffer(GeometryMeshRootParam::MatCB, objectCB);
        primitves[mesh].Draw(*commandList);
    }

    /*
    auto& view = game->registry.view<TransformComponent, StaticMeshComponent>();
    for (auto [entity, transform, sm] : view.each())
    {
        mat.model = transform.GetTransform();
        mat.mvp = mat.model * mat.view * mat.proj;
        mat.invTransposeMvp = XMMatrixInverse(nullptr, XMMatrixTranspose(mat.mvp));

        commandList->SetGraphicsDynamicConstantBuffer(GeometryMeshRootParam::MatCB, mat);
               
        for (int i = sm.startOffset; i < sm.startOffset + sm.count; i++)
        {
            auto& mesh = meshes[i];
            auto& material = mesh.m_Material;
            auto& textures = mesh.m_Material.textures;

            if (material.HasTexture(MaterialType::Diffuse))
            {    
                commandList->SetShaderResourceView(
                    GeometryMeshRootParam::Textures, 0, 
                    textures[MaterialType::Diffuse],
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);                              
            }
            
            mesh.Draw(*commandList);          
        } 
    }
    */

    commandQueue->ExecuteCommandList(commandList);
}

void DeferredRenderer::OnResize(ResizeEventArgs& e)
{
    if (m_Width == e.Width && m_Height == e.Height) return;

#ifdef DEBUG_EDITOR
    Application::Get().Flush();
#endif

    m_Width = e.Width;
    m_Height = e.Height;

    m_GBufferRenderTarget.Resize(m_Width, m_Height);
}
