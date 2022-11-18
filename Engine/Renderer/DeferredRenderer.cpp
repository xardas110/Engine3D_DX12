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
#include <TypesCompat.h>
#include <WinPixEventRuntime/pix3.h>

using namespace DirectX;

bool IsDirectXRaytracingSupported(IDXGIAdapter4* adapter)
{
    ComPtr<ID3D12Device> testDevice;
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};
    featureSupportData.RaytracingTier = D3D12_RAYTRACING_TIER_1_1;

    return SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))
        && SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
        && featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}

DeferredRenderer::DeferredRenderer(int width, int height)
    : m_Width(width), m_Height(height), m_GBuffer(m_Width, m_Height),
    m_LightPass(m_Width, m_Height),
    m_CompositionPass(m_Width, m_Height),
    m_DebugTexturePass(m_Width, m_Height)
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

    auto& srvHeap = assetManager->m_SrvHeapData;
    auto& globalMeshInfo = assetManager->m_MeshManager.instanceData.meshInfo;
    auto& globalMaterialInfo = assetManager->m_MaterialManager.instanceData.gpuInfo;
    auto& globalMaterialInfoCPU = assetManager->m_MaterialManager.instanceData.cpuInfo;
    auto& materials = assetManager->m_MaterialManager.materialData.materials;
    auto& meshes = assetManager->m_MeshManager.meshData.meshes;
    auto& meshInstance = assetManager->m_MeshManager.instanceData;
    auto& textures = assetManager->m_TextureManager.textureData.textures;

    auto* camera = game->GetRenderCamera();

    ObjectCB objectCB; //todo move to cam
    objectCB.view = camera->get_ViewMatrix();
    objectCB.proj = camera->get_ProjectionMatrix();
    objectCB.invView = XMMatrixInverse(nullptr, objectCB.view);
    objectCB.invProj = XMMatrixInverse(nullptr, objectCB.proj);

    CameraCB cameraCB;
    cameraCB.view = objectCB.view;
    cameraCB.proj = objectCB.proj;
    cameraCB.invView = objectCB.invView;
    cameraCB.invProj = objectCB.invProj;

    auto& directionalLight = game->m_DirectionalLight;

    // Clear the render targets.
    {
        PIXBeginEvent(commandList->GetGraphicsCommandList().Get(), 0, L"ClearGBuffer");
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        m_GBuffer.ClearRendetTarget(*commandList, clearColor);
        m_LightPass.ClearRendetTarget(*commandList, clearColor);
        PIXEndEvent(commandList->GetGraphicsCommandList().Get());
    }

    {// BUILD DXR STRUCTURE
        PIXBeginEvent(dxrCommandList->GetGraphicsCommandList().Get(), 0, L"Building DXR structure");

        m_Raytracer->BuildAccelerationStructure(*dxrCommandList, game->registry, Application::Get().GetAssetManager()->m_MeshManager, window.m_CurrentBackBufferIndex);

        PIXEndEvent(dxrCommandList->GetGraphicsCommandList().Get());       

        {//Compute execute
            PIXBeginEvent(computeQueue->GetD3D12CommandQueue().Get(), 0, L"ComputeQue DXR execute");
            //EXECUTING RTX STRUCTURE BUILDING
            computeQueue->ExecuteCommandList(dxrCommandList);
            PIXEndEvent(computeQueue->GetD3D12CommandQueue().Get());
        }
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
        commandList->SetGraphicsDynamicStructuredBuffer(GBufferParam::GlobalMaterials, materials);
        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, srvHeap.heap.Get());
        commandList->GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(GBufferParam::GlobalHeapData, srvHeap.heap->GetGPUDescriptorHandleForHeapStart());
        commandList->SetGraphicsDynamicStructuredBuffer(GBufferParam::GlobalMatInfo, globalMaterialInfo);

        commandList->TransitionBarrier(m_GBuffer.renderTarget.GetTexture(AttachmentPoint::Color0), D3D12_RESOURCE_STATE_RENDER_TARGET);

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

            auto matInstanceID = globalMeshInfo[mesh.id].materialInstanceID;
            objectCB.materialGPUID = matInstanceID;

            commandList->SetGraphicsDynamicConstantBuffer(GBufferParam::ObjectCB, objectCB);

            meshes[meshInstance.meshIds[mesh.id]].mesh.Draw(*commandList);   
        }
        PIXEndEvent(commandList->GetGraphicsCommandList().Get());
    }
    {
        PIXBeginEvent(commandList->GetGraphicsCommandList().Get(), 0, L"LightPass");
        commandList->SetRenderTarget(m_LightPass.renderTarget);
        commandList->SetViewport(m_LightPass.renderTarget.GetViewport());
        commandList->SetScissorRect(m_ScissorRect);
        commandList->SetPipelineState(m_LightPass.pipeline);
        commandList->SetGraphicsRootSignature(m_LightPass.rootSignature);
        commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, srvHeap.heap.Get());

        commandList->GetGraphicsCommandList()->SetGraphicsRootShaderResourceView(
            LightPassParam::AccelerationStructure,
            m_Raytracer->GetCurrentTLAS()->GetGPUVirtualAddress());

        commandList->GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(LightPassParam::GlobalHeapData, srvHeap.heap->GetGPUDescriptorHandleForHeapStart());

        commandList->SetGraphicsDynamicStructuredBuffer(LightPassParam::GlobalMaterials, materials);

        commandList->SetGraphicsDynamicStructuredBuffer(LightPassParam::GlobalMatInfo, globalMaterialInfo);

        commandList->SetGraphicsDynamicStructuredBuffer(LightPassParam::GlobalMeshInfo, globalMeshInfo);

        commandList->TransitionBarrier(m_GBuffer.renderTarget.GetTexture(AttachmentPoint::Color0), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        auto gBufferHeap = m_GBuffer.CreateSRVViews();

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_GBuffer.m_SRVHeap.heap.Get());

        commandList->GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(LightPassParam::GBufferHeap, gBufferHeap);

        commandList->SetGraphicsDynamicConstantBuffer(LightPassParam::CameraCB, cameraCB);
        commandList->SetGraphicsDynamicConstantBuffer(LightPassParam::DirectionalLightCB, directionalLight);

        commandList->Draw(3);

        PIXEndEvent(commandList->GetGraphicsCommandList().Get());
    }
    {//Composition branch
        PIXBeginEvent(commandList->GetGraphicsCommandList().Get(), 0, L"CompositionPass");
        commandList->SetRenderTarget(m_CompositionPass.renderTarget);
        commandList->SetViewport(m_CompositionPass.renderTarget.GetViewport());
        commandList->SetScissorRect(m_ScissorRect);
        commandList->SetPipelineState(m_CompositionPass.pipeline);
        commandList->SetGraphicsRootSignature(m_CompositionPass.rootSignature);
        commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, srvHeap.heap.Get());

        commandList->GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(CompositionPassParam::GlobalHeapData, srvHeap.heap->GetGPUDescriptorHandleForHeapStart());

        commandList->SetGraphicsDynamicStructuredBuffer(CompositionPassParam::GlobalMaterials, materials);

        commandList->SetGraphicsDynamicStructuredBuffer(CompositionPassParam::GlobalMatInfo, globalMaterialInfo);

        commandList->SetGraphicsDynamicStructuredBuffer(CompositionPassParam::GlobalMeshInfo, globalMeshInfo);

        commandList->TransitionBarrier(m_GBuffer.renderTarget.GetTexture(AttachmentPoint::Color0), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        auto gBufferHeap = m_GBuffer.CreateSRVViews();
        
        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_GBuffer.m_SRVHeap.heap.Get());

        commandList->GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(CompositionPassParam::GBufferHeap, gBufferHeap);

        auto lightMapView = m_LightPass.CreateSRVViews();
        auto lightMapHeap = m_LightPass.m_SRVHeap;

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, lightMapHeap.heap.Get());

        commandList->GetGraphicsCommandList()->SetGraphicsRootShaderResourceView(
            CompositionPassParam::AccelerationStructure,
            m_Raytracer->GetCurrentTLAS()->GetGPUVirtualAddress());

        commandList->GetGraphicsCommandList()->SetGraphicsRootDescriptorTable(CompositionPassParam::LightMapHeap, lightMapView);

        commandList->Draw(3);
        PIXEndEvent(commandList->GetGraphicsCommandList().Get());
    }
    { //Debug pass
        commandList->SetRenderTarget(m_DebugTexturePass.renderTarget);
        commandList->SetViewport(m_DebugTexturePass.renderTarget.GetViewport());
        commandList->SetScissorRect(m_ScissorRect);
        commandList->SetPipelineState(m_DebugTexturePass.pipeline);
        commandList->SetGraphicsRootSignature(m_DebugTexturePass.rootSignature);
        commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        const char* listbox_items[] =
        { "FinalColor", "GBufferAlbedo", "GBufferNormal",
            "GBufferPBR", "GBufferEmissive", "DirectDiffuse",
            "IndirectDiffuse", "IndirectSpecular" };

        const Texture* texArray[] =
        {
            &m_CompositionPass.renderTarget.GetTexture(AttachmentPoint::Color0),
            &m_GBuffer.renderTarget.GetTexture(AttachmentPoint::Color0),
            &m_GBuffer.renderTarget.GetTexture(AttachmentPoint::Color1),
            &m_GBuffer.renderTarget.GetTexture(AttachmentPoint::Color2),
            &m_GBuffer.renderTarget.GetTexture(AttachmentPoint::Color3),
            &m_LightPass.renderTarget.GetTexture(AttachmentPoint::Color0),
            &m_LightPass.renderTarget.GetTexture(AttachmentPoint::Color1),
            &m_LightPass.renderTarget.GetTexture(AttachmentPoint::Color2),
        };
        ImGui::Begin("Select render buffer");
        static int listbox_item_current = 0;
        ImGui::ListBox("listbox\n(single select)", &listbox_item_current, listbox_items, IM_ARRAYSIZE(listbox_items));
        ImGui::End();

        commandList->SetShaderResourceView(DebugTextureParam::texture, 0, *texArray[listbox_item_current]);

        commandList->Draw(3);
    }
    {//Graphics execute
        PIXBeginEvent(graphicsQueue->GetD3D12CommandQueue().Get(), 0, L"Graphics execute");
        std::vector<std::shared_ptr<CommandList>> commandLists;
        //commandLists.emplace_back(dxrCommandList);       
        commandLists.emplace_back(commandList);   
        //commandLists.emplace_back(commandList1);
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
    m_LightPass.OnResize(m_Width, m_Height);
    m_CompositionPass.OnResize(m_Width, m_Height);
    m_DebugTexturePass.OnResize(m_Width, m_Height);
}
