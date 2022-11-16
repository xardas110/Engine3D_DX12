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
#include <WinPixEventRuntime/pix3.h>


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
    :m_Width(width), m_Height(height), m_GBuffer(m_Width, m_Height)
{
    //auto adapter = Application::Get().GetAdapter(false);  
    //assert(IsDirectXRaytracingSupported(adapter.Get()));
    std::cout << "DeferredRenderer: Raytracing is supported on the device" << std::endl;

    m_Raytracer = std::unique_ptr<Raytracing>(new Raytracing());
    m_Raytracer->Init();
}

DeferredRenderer::~DeferredRenderer()
{
}


void DeferredRenderer::Render(Window& window)
{
    if (!window.m_pGame.lock()) return;
    
    auto game = window.m_pGame.lock();

    auto graphicsQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto computeQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    auto commandList = graphicsQueue->GetCommandList();
    auto dxrCommandList = computeQueue->GetCommandList();

    auto assetManager = Application::Get().GetAssetManager();

    auto& srvHeap = assetManager->m_SrvHeapData.heap;
    auto& globalMeshInfo = assetManager->m_MeshManager.instanceData.meshInfo;
    auto& globalMaterialInfo = assetManager->m_MaterialManager.instanceData.gpuInfo;
    auto& globalMaterialInfoCPU = assetManager->m_MaterialManager.instanceData.cpuInfo;
    auto& meshes = assetManager->m_MeshManager.meshData.meshes;
    auto& meshInstance = assetManager->m_MeshManager.instanceData;
    auto& textures = assetManager->m_TextureManager.textureData.textures;

    auto* camera = game->GetRenderCamera();

    ObjectCB objectCB;
    objectCB.view = camera->get_ViewMatrix();
    objectCB.proj = camera->get_ProjectionMatrix();
    objectCB.invView = XMMatrixInverse(nullptr, objectCB.view);
    objectCB.invProj = XMMatrixInverse(nullptr, objectCB.proj);

    // Clear the render targets.
    {
        PIXBeginEvent(commandList->GetGraphicsCommandList().Get(), 0, L"ClearGBuffer");
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        m_GBuffer.ClearRendetTarget(*commandList, clearColor);
        PIXEndEvent(commandList->GetGraphicsCommandList().Get());
    }

    {// BUILD DXR STRUCTURE
        PIXBeginEvent(dxrCommandList->GetGraphicsCommandList().Get(), 0, L"Building DXR structure");

        m_Raytracer->BuildAccelerationStructure(*dxrCommandList, game->registry, Application::Get().GetAssetManager()->m_MeshManager, window.m_CurrentBackBufferIndex);

        PIXEndEvent(dxrCommandList->GetGraphicsCommandList().Get());       
    }

    //DEPTH PREPASS
    {
        PIXBeginEvent(commandList->GetGraphicsCommandList().Get(), 0, L"zPrePass");

        commandList->SetRenderTarget(m_GBuffer.renderTarget);
        commandList->SetViewport(m_GBuffer.renderTarget.GetViewport());
        commandList->SetScissorRect(m_ScissorRect);
        commandList->SetPipelineState(m_GBuffer.zPrePassPipeline);
        commandList->SetGraphicsRootSignature(m_GBuffer.zPrePassRS);

        auto& view = game->registry.view<TransformComponent, MeshComponent>();
        for (auto [entity, transform, mesh] : view.each())
        {

            objectCB.model = transform.GetTransform();
            objectCB.mvp = objectCB.model * objectCB.view * objectCB.proj;
            objectCB.invTransposeMvp = XMMatrixInverse(nullptr, XMMatrixTranspose(objectCB.mvp));
            objectCB.entId = (UINT)entity;
            objectCB.meshId = mesh.id;

            objectCB.transposeInverseModel = (XMMatrixInverse(nullptr, XMMatrixTranspose(objectCB.model)));
            globalMeshInfo[mesh.id].objRot = transform.rot;

            auto matIndex = globalMeshInfo[mesh.id].materialIndex;
            auto albedoIndex = globalMaterialInfoCPU[matIndex].albedo;

            commandList->SetGraphicsDynamicConstantBuffer(DepthPrePassParam::ObjectCB, objectCB);

            meshes[meshInstance.meshIds[mesh.id]].mesh.Draw(*commandList);
        }
        PIXEndEvent(commandList->GetGraphicsCommandList().Get());
    }

    /*
    commandList->GetGraphicsCommandList()->SetGraphicsRootShaderResourceView(
        GlobalRootParam::AccelerationStructure, 
        m_Raytracer->GetCurrentTLAS()->GetGPUVirtualAddress());
    
    commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, srvHeap.Get());
    commandList->GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(GlobalRootParam::GlobalHeapData, srvHeap->GetGPUDescriptorHandleForHeapStart());

    commandList->SetGraphicsDynamicStructuredBuffer(GlobalRootParam::GlobalMeshInfo, globalMeshInfo);
    commandList->SetGraphicsDynamicStructuredBuffer(GlobalRootParam::GlobalMaterialInfo, globalMaterialInfo);
  */
    {//GBuffer Pass
        PIXBeginEvent(commandList->GetGraphicsCommandList().Get(), 0, L"GBufferPass");
        commandList->SetRenderTarget(m_GBuffer.renderTarget);
        commandList->SetViewport(m_GBuffer.renderTarget.GetViewport());
        commandList->SetScissorRect(m_ScissorRect);
        commandList->SetPipelineState(m_GBuffer.pipeline);
        commandList->SetGraphicsRootSignature(m_GBuffer.rootSignature);

        auto& view = game->registry.view<TransformComponent, MeshComponent>();
        for (auto [entity, transform, mesh] : view.each())
        {
        
            objectCB.model = transform.GetTransform();
            objectCB.mvp = objectCB.model * objectCB.view * objectCB.proj;
            objectCB.invTransposeMvp = XMMatrixInverse(nullptr, XMMatrixTranspose(objectCB.mvp));
            objectCB.entId = (UINT)entity;
            objectCB.meshId = mesh.id;
        
            objectCB.transposeInverseModel = (XMMatrixInverse(nullptr, XMMatrixTranspose(objectCB.model)));
            globalMeshInfo[mesh.id].objRot = transform.rot;

            auto matIndex = globalMeshInfo[mesh.id].materialIndex;
            auto albedoIndex = globalMaterialInfoCPU[matIndex].albedo;

            commandList->SetShaderResourceView(
            GBufferParam::Textures,
                0,
                textures[albedoIndex].texture,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            commandList->SetGraphicsDynamicConstantBuffer(GBufferParam::ObjectCB, objectCB);

            meshes[meshInstance.meshIds[mesh.id]].mesh.Draw(*commandList);   
        }
        PIXEndEvent(commandList->GetGraphicsCommandList().Get());
    }
    {//Compute execute
        PIXBeginEvent(computeQueue->GetD3D12CommandQueue().Get(), 0, L"ComputeQue DXR execute");
        //EXECUTING RTX STRUCTURE BUILDING
        computeQueue->ExecuteCommandList(dxrCommandList);
        PIXEndEvent(computeQueue->GetD3D12CommandQueue().Get());
    }
    {//Graphics execute
        PIXBeginEvent(graphicsQueue->GetD3D12CommandQueue().Get(), 0, L"Graphics execute");
        std::vector<std::shared_ptr<CommandList>> commandLists;
        //commandLists.emplace_back(dxrCommandList);
        commandLists.emplace_back(commandList);   
        graphicsQueue->ExecuteCommandLists(commandLists); 
        PIXEndEvent(graphicsQueue->GetD3D12CommandQueue().Get());
    }  
}

void DeferredRenderer::OnResize(ResizeEventArgs& e)
{
    if (m_Width == e.Width && m_Height == e.Height) return;

#ifdef DEBUG_EDITOR
    Application::Get().Flush();
#endif

    m_Width = e.Width;
    m_Height = e.Height;

    m_GBuffer.OnResize(m_Width, m_Height);
}
