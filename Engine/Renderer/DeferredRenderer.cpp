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

//#include <NRIDescs.h>
//#include <Extensions/NRIWrapperD3D12.h>
//#include <Extensions/NRIHelper.h>

//#include <NRD.h>
//#include <NRDIntegration.h>

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
    : m_Width(width), m_Height(height)
{
    m_NativeWidth = width;
    m_NativeHeight = height;

    m_DLSS = std::unique_ptr<DLSS>(new DLSS(width, height));
    auto dlssRes = m_DLSS->recommendedSettings.m_ngxRecommendedOptimalRenderSize;

    if (m_DLSS->bDlssOn)
    {
        m_Width = dlssRes.x;
        m_Height = dlssRes.y;
    }
    else
    {
        m_Width = width;
        m_Height = height;
    }

    m_GBuffer = std::unique_ptr<GBuffer>(new GBuffer(m_Width, m_Height));
    m_LightPass = std::unique_ptr<LightPass>(new LightPass(m_Width, m_Height));
    m_CompositionPass = std::unique_ptr<CompositionPass>(new CompositionPass(m_Width, m_Height));
    m_DebugTexturePass = std::unique_ptr<DebugTexturePass>(new DebugTexturePass(m_Width, m_Height));

    m_HDR = std::unique_ptr<HDR>(new HDR(m_NativeWidth, m_NativeHeight));

    //auto adapter = Application::Get().GetAdapter(false);  
    //assert(IsDirectXRaytracingSupported(adapter.Get()));
    std::cout << "DeferredRenderer: Raytracing is supported on the device" << std::endl;

    m_Raytracer = std::unique_ptr<Raytracing>(new Raytracing());
    m_Raytracer->Init();

    m_NvidiaDenoiser = std::unique_ptr<NvidiaDenoiser>(new NvidiaDenoiser(m_Width, m_Height, GetDenoiserTextures()));

    m_Skybox = std::unique_ptr<Skybox>(new Skybox(L"Assets/Textures/belfast_sunset_puresky_4k.hdr"));


    m_DLSS->dlssChangeEvent.attach(&DeferredRenderer::OnDlssChanged, this);

}

DeferredRenderer::~DeferredRenderer()
{
    m_DLSS->dlssChangeEvent.detach(&DeferredRenderer::OnDlssChanged, this);
}

std::vector<MeshInstanceWrapper> DeferredRenderer::GetMeshInstances(entt::registry& registry)
{
    auto& meshManager = Application::Get().GetAssetManager()->m_MeshManager;
    std::vector<MeshInstanceWrapper> instances;
    {
        auto& view = registry.view<TransformComponent, MeshComponent>();
        for (auto [entity, transform, mesh] : view.each())
        {
            MeshInstanceWrapper wrap(transform, mesh);
            instances.emplace_back(wrap);
        }
    }
    {
        auto& view = registry.view<TransformComponent, StaticMeshComponent>();
        for (auto [entity, transform, sm] : view.each())
        {
            for (size_t i = sm.startOffset; i < sm.endOffset; i++)
            {
                MeshInstanceWrapper wrap(transform, i);
                instances.emplace_back(wrap);
            }
        }
    }
    return instances;
}

DenoiserTextures DeferredRenderer::GetDenoiserTextures()
{
    DenoiserTextures dt(
        m_GBuffer->GetTexture(GBUFFER_MOTION_VECTOR),
        m_GBuffer->GetTexture(GBUFFER_LINEAR_DEPTH),
        m_GBuffer->GetTexture(GBUFFER_NORMAL_ROUGHNESS),
        m_LightPass->GetTexture(LIGHTBUFFER_INDIRECT_DIFFUSE),
        m_LightPass->GetTexture(LIGHTBUFFER_INDIRECT_SPECULAR),
        m_LightPass->GetTexture(LIGHTBUFFER_DENOISED_INDIRECT_DIFFUSE),
        m_LightPass->GetTexture(LIGHTBUFFER_DENOISED_INDIRECT_SPECULAR)
    );
    return dt;
}

void DeferredRenderer::Render(Window& window)
{
    if (!window.m_pGame.lock()) return;
 
    HDR::UpdateGUI();
    m_DLSS->OnGUI();

    static int listbox_item_debug = 0;

    auto game = window.m_pGame.lock();

    auto graphicsQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = graphicsQueue->GetCommandList();
    auto gfxCommandList = commandList->GetGraphicsCommandList();

    auto assetManager = Application::Get().GetAssetManager();

    auto& srvHeap = assetManager->m_SrvHeapData;
    auto& globalMeshInfo = assetManager->m_MeshManager.instanceData.meshInfo;
    auto& globalMaterialInfo = assetManager->m_MaterialManager.instanceData.gpuInfo;
    auto& globalMaterialInfoCPU = assetManager->m_MaterialManager.instanceData.cpuInfo;
    auto& materials = assetManager->m_MaterialManager.materialData.materials;
    auto& meshInstance = assetManager->m_MeshManager.instanceData;
    auto& textures = assetManager->m_TextureManager.textureData.textures;
    auto meshInstances = GetMeshInstances(game->registry);

    prevTrans.resize(meshInstances.size());

    auto* camera = game->GetRenderCamera();

    ObjectCB objectCB; //todo move to cam
    objectCB.view = camera->get_ViewMatrix();
    objectCB.proj = camera->get_ProjectionMatrix();
    objectCB.invView = XMMatrixInverse(nullptr, objectCB.view);
    objectCB.invProj = XMMatrixInverse(nullptr, objectCB.proj);
      
    cameraCB.view = camera->get_ViewMatrix();
    cameraCB.proj = camera->get_ProjectionMatrix();
    cameraCB.invView = XMMatrixInverse(nullptr, cameraCB.view);
    cameraCB.invProj = XMMatrixInverse(nullptr, cameraCB.proj);
    cameraCB.viewProj = cameraCB.view * cameraCB.proj;
    cameraCB.invViewProj = XMMatrixInverse(nullptr, cameraCB.viewProj);
    cameraCB.resolution = { (float)m_Width, (float)m_Height };
    cameraCB.zNear = camera->GetNear();
    cameraCB.zFar = camera->GetFar();

    RaytracingDataCB rtData;
    rtData.frameNumber = Application::GetFrameCount();
    rtData.debugSettings = listbox_item_debug;

    static int numBounces = 2;

    ImGui::Begin("Raytracing Settings");
    ImGui::SliderInt("Num Bounces", &numBounces, 0, 10);
    rtData.numBounces = numBounces;
    ImGui::End();

    XMStoreFloat3(&cameraCB.pos, camera->get_Translation());

    auto& directionalLight = game->m_DirectionalLight;

    ImGui::Begin("Directional light settings");
    if (ImGui::SliderFloat3("Direction", &directionalLight.data.direction.m128_f32[0], -1, +1))
    {
        if (XMVector3Equal(directionalLight.data.direction, XMVectorZero()))
        { 
            directionalLight.data.direction = { 0.f, -1.f, 0.f, directionalLight.data.direction.m128_f32[3] };
        }        
        else
        { 
            directionalLight.data.direction = XMVector3Normalize(directionalLight.data.direction);
        }
    }
    ImGui::SliderFloat("Lux", &directionalLight.data.color.m128_f32[3], 0, 20.f);
    ImGui::ColorPicker3("Color", &directionalLight.data.color.m128_f32[0], ImGuiColorEditFlags_Float);    
    ImGui::End();

    // Clear the render targets.
    {
        PIXBeginEvent(gfxCommandList.Get(), 0, L"Clear buffers");
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        commandList->SetRenderTarget(m_GBuffer->renderTarget);
        m_GBuffer->ClearRendetTarget(*commandList, clearColor);

        commandList->SetRenderTarget(m_LightPass->renderTarget);
        m_LightPass->ClearRendetTarget(*commandList, clearColor);

        PIXEndEvent(gfxCommandList.Get());
    }
    {// BUILD DXR STRUCTURE
        PIXBeginEvent(gfxCommandList.Get(), 0, L"Building DXR structure");    
        m_Raytracer->BuildAccelerationStructure(*commandList, meshInstances, Application::Get().GetAssetManager()->m_MeshManager, window.m_CurrentBackBufferIndex);      
        PIXEndEvent(gfxCommandList.Get());
    }  
    {//DEPTH PREPASS
        PIXBeginEvent(gfxCommandList.Get(), 0, L"zPrePass");

        commandList->SetRenderTarget(m_GBuffer->renderTarget);
        commandList->SetViewport(m_GBuffer->renderTarget.GetViewport());
        commandList->SetScissorRect(m_ScissorRect);
        commandList->SetPipelineState(m_GBuffer->zPrePassPipeline);
        commandList->SetGraphicsRootSignature(m_GBuffer->zPrePassRS);

        for (auto [transform, mesh] : meshInstances)
        {       
            objectCB.model = transform.GetTransform();
            objectCB.mvp = objectCB.model * objectCB.view * objectCB.proj;
            objectCB.invTransposeMvp = XMMatrixInverse(nullptr, XMMatrixTranspose(objectCB.mvp));
            objectCB.meshId = mesh.id;

            objectCB.transposeInverseModel = (XMMatrixInverse(nullptr, XMMatrixTranspose(objectCB.model)));
            globalMeshInfo[mesh.id].objRot = transform.rot;

            commandList->SetGraphicsDynamicConstantBuffer(DepthPrePassParam::ObjectCB, objectCB);

            assetManager->m_MeshManager.meshData.meshes[meshInstance.meshIds[mesh.id]].mesh.Draw(*commandList);
        }
        PIXEndEvent(gfxCommandList.Get());
    }
    {//GBuffer Pass
        PIXBeginEvent(gfxCommandList.Get(), 0, L"GBufferPass");
        commandList->SetRenderTarget(m_GBuffer->renderTarget);
        commandList->SetViewport(m_GBuffer->renderTarget.GetViewport());
        commandList->SetScissorRect(m_ScissorRect);
        commandList->SetPipelineState(m_GBuffer->pipeline);

        gfxCommandList->SetGraphicsRootSignature(m_GBuffer->rootSignature.GetRootSignature().Get());
        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, srvHeap.heap.Get());
        gfxCommandList->SetGraphicsRootDescriptorTable(GBufferParam::GlobalHeapData, srvHeap.heap->GetGPUDescriptorHandleForHeapStart());

        commandList->SetGraphicsDynamicStructuredBuffer(GBufferParam::GlobalMaterials, materials);       
        commandList->SetGraphicsDynamicStructuredBuffer(GBufferParam::GlobalMatInfo, globalMaterialInfo);
        commandList->SetGraphicsDynamicConstantBuffer(GBufferParam::CameraCB, cameraCB);

        int i = 0;
        for (auto [transform, mesh] : meshInstances)
        {       
            objectCB.model = transform.GetTransform();

            objectCB.mvp = objectCB.model * objectCB.view * objectCB.proj;
            objectCB.prevMVP = prevTrans[i].GetTransform() * cameraCB.prevView * cameraCB.prevProj;

            objectCB.invTransposeMvp = XMMatrixInverse(nullptr, XMMatrixTranspose(objectCB.mvp));
            objectCB.meshId = mesh.id;

            objectCB.prevModel = prevTrans[i++].GetTransform();
            objectCB.transposeInverseModel = (XMMatrixInverse(nullptr, XMMatrixTranspose(objectCB.model)));

            globalMeshInfo[mesh.id].objRot = transform.rot;

            auto matInstanceID = globalMeshInfo[mesh.id].materialInstanceID;
            objectCB.materialGPUID = matInstanceID;

            commandList->SetGraphicsDynamicConstantBuffer(GBufferParam::ObjectCB, objectCB);

            assetManager->m_MeshManager.meshData.meshes[meshInstance.meshIds[mesh.id]].mesh.Draw(*commandList);
        }

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_ALBEDO).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_AO_METALLIC_HEIGHT).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_EMISSIVE_SHADER_MODEL).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_LINEAR_DEPTH).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_STANDARD_DEPTH).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_MOTION_VECTOR).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_NORMAL_ROUGHNESS).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_GEOMETRY_NORMAL).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_GEOMETRY_MV2D).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        PIXEndEvent(gfxCommandList.Get());
    }
    {//LIGHT PASS
        PIXBeginEvent(gfxCommandList.Get(), 0, L"LightPass");

        commandList->SetRenderTarget(m_LightPass->renderTarget);
        commandList->SetViewport(m_LightPass->renderTarget.GetViewport());
        commandList->SetScissorRect(m_ScissorRect);
        commandList->SetPipelineState(m_LightPass->pipeline);
        gfxCommandList->SetGraphicsRootSignature(m_LightPass->rootSignature.GetRootSignature().Get());

        commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        
        gfxCommandList->SetGraphicsRootShaderResourceView(
            LightPassParam::AccelerationStructure,
            m_Raytracer->GetCurrentTLAS()->GetGPUVirtualAddress());

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, srvHeap.heap.Get());

        gfxCommandList->SetGraphicsRootDescriptorTable(LightPassParam::GlobalHeapData, srvHeap.heap->GetGPUDescriptorHandleForHeapStart());

        commandList->SetGraphicsDynamicStructuredBuffer(LightPassParam::GlobalMaterials, materials);

        commandList->SetGraphicsDynamicStructuredBuffer(LightPassParam::GlobalMatInfo, globalMaterialInfo);

        commandList->SetGraphicsDynamicStructuredBuffer(LightPassParam::GlobalMeshInfo, globalMeshInfo);

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_GBuffer->m_SRVHeap.heap.Get());
        gfxCommandList->SetGraphicsRootDescriptorTable(LightPassParam::GBufferHeap, m_GBuffer->m_SRVHeap.GetGPUHandle(0));

        commandList->SetGraphicsDynamicConstantBuffer(LightPassParam::CameraCB, cameraCB);
        commandList->SetGraphicsDynamicConstantBuffer(LightPassParam::DirectionalLightCB, directionalLight);
        commandList->SetGraphicsDynamicConstantBuffer(LightPassParam::RaytracingDataCB, rtData);

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_Skybox->heap.heap.Get());
        gfxCommandList->SetGraphicsRootDescriptorTable(LightPassParam::Cubemap, m_Skybox->GetSRVView());

        commandList->Draw(3);

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->GetTexture(LIGHTBUFFER_DIRECT_LIGHT).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->GetTexture(LIGHTBUFFER_INDIRECT_DIFFUSE).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->GetTexture(LIGHTBUFFER_INDIRECT_SPECULAR).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->GetTexture(LIGHTBUFFER_RT_DEBUG).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->GetTexture(LIGHTBUFFER_ACCUM_BUFFER).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        PIXEndEvent(gfxCommandList.Get());
    }
    {//Denoising 
        PIXBeginEvent(gfxCommandList.Get(), 0, L"Denoising Pass");

        DenoiserTextures dt = GetDenoiserTextures();

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_GBuffer->m_SRVHeap.heap.Get());

        gfxCommandList->ResourceBarrier(1, 
            &CD3DX12_RESOURCE_BARRIER::Transition(
            dt.inMV.GetD3D12Resource().Get(), 
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                dt.inNormalRough.GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                dt.inViewZ.GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_LightPass->m_SRVHeap.heap.Get());

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                dt.inIndirectDiffuse.GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1, 
            &CD3DX12_RESOURCE_BARRIER::Transition(
            dt.inIndirectSpecular.GetD3D12Resource().Get(), 
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                dt.outIndirectDiffuse.GetD3D12Resource().Get(), 
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                dt.outIndirectSpecular.GetD3D12Resource().Get(), 
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

        m_NvidiaDenoiser->RenderFrame(*commandList, cameraCB, window.m_CurrentBackBufferIndex, m_Width, m_Height);

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_GBuffer->m_SRVHeap.heap.Get());

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                dt.inMV.GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                dt.inNormalRough.GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                dt.inViewZ.GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_LightPass->m_SRVHeap.heap.Get());

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                dt.inIndirectDiffuse.GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                dt.inIndirectSpecular.GetD3D12Resource().Get(), 
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                dt.outIndirectDiffuse.GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                dt.outIndirectSpecular.GetD3D12Resource().Get(), 
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        PIXEndEvent(gfxCommandList.Get());
    }
    {//Composition branch
        PIXBeginEvent(gfxCommandList.Get(), 0, L"CompositionPass");

        commandList->SetRenderTarget(m_CompositionPass->renderTarget);
        commandList->SetViewport(m_CompositionPass->renderTarget.GetViewport());
        commandList->SetScissorRect(m_ScissorRect);
        commandList->SetPipelineState(m_CompositionPass->pipeline);
        gfxCommandList->SetGraphicsRootSignature(m_CompositionPass->rootSignature.GetRootSignature().Get());
        commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, srvHeap.heap.Get());

        gfxCommandList->SetGraphicsRootDescriptorTable(CompositionPassParam::GlobalHeapData, srvHeap.heap->GetGPUDescriptorHandleForHeapStart());

        commandList->SetGraphicsDynamicStructuredBuffer(CompositionPassParam::GlobalMaterials, materials);

        commandList->SetGraphicsDynamicStructuredBuffer(CompositionPassParam::GlobalMatInfo, globalMaterialInfo);

        commandList->SetGraphicsDynamicStructuredBuffer(CompositionPassParam::GlobalMeshInfo, globalMeshInfo);
  
        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_GBuffer->m_SRVHeap.heap.Get());

        gfxCommandList->SetGraphicsRootDescriptorTable(CompositionPassParam::GBufferHeap, m_GBuffer->m_SRVHeap.GetGPUHandle(0));

        gfxCommandList->SetGraphicsRootShaderResourceView(
            CompositionPassParam::AccelerationStructure,
            m_Raytracer->GetCurrentTLAS()->GetGPUVirtualAddress());

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_LightPass->m_SRVHeap.heap.Get());
        gfxCommandList->SetGraphicsRootDescriptorTable(CompositionPassParam::LightMapHeap, m_LightPass->m_SRVHeap.GetGPUHandle(0));

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_Skybox->heap.heap.Get());
        gfxCommandList->SetGraphicsRootDescriptorTable(CompositionPassParam::Cubemap, m_Skybox->GetSRVView());

        commandList->SetGraphicsDynamicConstantBuffer(CompositionPassParam::RaytracingDataCB, rtData);
        commandList->SetGraphicsDynamicConstantBuffer(CompositionPassParam::CameraCB, cameraCB);
        commandList->SetGraphicsDynamicConstantBuffer(CompositionPassParam::TonemapCB, HDR::GetTonemapCB());

        commandList->Draw(3);

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_CompositionPass->renderTarget.GetTexture(AttachmentPoint::Color0).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        PIXEndEvent(gfxCommandList.Get());
    } 
    { //DLSS pass

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_DLSS->resolvedTexture.GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS));


        if (m_DLSS->bDlssOn)
        { 
            PIXBeginEvent(gfxCommandList.Get(), 0, L"DLSS Pass");
            DlssTextures dlssTextures
            (
                &m_CompositionPass->renderTarget.GetTexture(AttachmentPoint::Color0),
                &m_DLSS->resolvedTexture,
                &m_GBuffer->GetTexture(GBUFFER_GEOMETRY_MV2D),
                &m_GBuffer->GetTexture(GBUFFER_STANDARD_DEPTH)
            );

            dlssTextures.linearDepth = &m_GBuffer->GetTexture(GBUFFER_LINEAR_DEPTH);
            dlssTextures.motionVectors3D = &m_GBuffer->GetTexture(GBUFFER_MOTION_VECTOR);


            m_DLSS->EvaluateSuperSampling(commandList.get(), dlssTextures, m_NativeWidth, m_NativeHeight);

            PIXEndEvent(gfxCommandList.Get());
        }

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_DLSS->resolvedTexture.GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    }
    { //HDR pass
        PIXBeginEvent(gfxCommandList.Get(), 0, L"HDR Pass");

        commandList->SetRenderTarget(m_HDR->renderTarget);
        commandList->SetViewport(m_HDR->renderTarget.GetViewport());
        commandList->SetScissorRect(m_ScissorRect);
        commandList->SetPipelineState(m_HDR->pipeline);
        gfxCommandList->SetGraphicsRootSignature(m_HDR->rootSignature.GetRootSignature().Get());
        commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        if (m_DLSS->bDlssOn)
        {
            commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_DLSS->heap.heap.Get());
            gfxCommandList->SetGraphicsRootDescriptorTable(HDRParam::ColorTexture, m_DLSS->heap.GetGPUHandle(0));
        }
        else
        {
            commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_CompositionPass->heap.heap.Get());
            gfxCommandList->SetGraphicsRootDescriptorTable(HDRParam::ColorTexture, m_CompositionPass->heap.GetGPUHandle(0));
        }

        commandList->SetGraphicsDynamicConstantBuffer(HDRParam::TonemapCB, HDR::GetTonemapCB());

        commandList->Draw(3);

        PIXEndEvent(gfxCommandList.Get());
    }
    { //Debug pass
        commandList->SetRenderTarget(m_DebugTexturePass->renderTarget);
        commandList->SetViewport(m_DebugTexturePass->renderTarget.GetViewport());
        commandList->SetScissorRect(m_ScissorRect);
        commandList->SetPipelineState(m_DebugTexturePass->pipeline);
        commandList->SetGraphicsRootSignature(m_DebugTexturePass->rootSignature);

        commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        const char* listbox_items[] =
        {   "Final Color", 
            "GBuffer Albedo", 
            "GBuffer Normal & Roughness",
            "GBuffer MotionVector", 
            "GBuffer Emissive & SM", 
            "GBuffer AO & Metallic & Height",
            "GBuffer LinearDepth", 
            "GBuffer Geometry Normal",
            "LightBuffer Direct Light",
            "LightBuffer IndirectDiffuse", 
            "LightBuffer IndirectSpecular", 
            "LightBuffer Denoised IndirectDiffuse",
            "LightBuffer Denoised IndirectSpecular", 
            "Raytraced Albedo", 
            "Raytraced Normal",
            "Raytraced AO", 
            "Raytraced Roughness", 
            "Raytraced Metallic", 
            "Raytraced Height",
            "Raytraced Emissive",
            "Raytraced Hit T",
            "GBuffer MV2D"
        };

        const Texture* texArray[] =
        {
            &m_HDR->renderTarget.GetTexture(AttachmentPoint::Color0),
            &m_GBuffer->GetTexture(GBUFFER_ALBEDO),
            &m_GBuffer->GetTexture(GBUFFER_NORMAL_ROUGHNESS),
            &m_GBuffer->GetTexture(GBUFFER_MOTION_VECTOR),
            &m_GBuffer->GetTexture(GBUFFER_EMISSIVE_SHADER_MODEL),
            &m_GBuffer->GetTexture(GBUFFER_AO_METALLIC_HEIGHT),
            &m_GBuffer->GetTexture(GBUFFER_LINEAR_DEPTH),
            &m_GBuffer->GetTexture(GBUFFER_GEOMETRY_NORMAL),
            &m_LightPass->GetTexture(LIGHTBUFFER_DIRECT_LIGHT),
            &m_LightPass->GetTexture(LIGHTBUFFER_INDIRECT_DIFFUSE),
            &m_LightPass->GetTexture(LIGHTBUFFER_INDIRECT_SPECULAR),
            &m_CompositionPass->renderTarget.GetTexture(AttachmentPoint::Color0),
            &m_CompositionPass->renderTarget.GetTexture(AttachmentPoint::Color0),
            &m_LightPass->GetTexture(LIGHTBUFFER_RT_DEBUG),
            &m_LightPass->GetTexture(LIGHTBUFFER_RT_DEBUG),
            &m_LightPass->GetTexture(LIGHTBUFFER_RT_DEBUG),
            &m_LightPass->GetTexture(LIGHTBUFFER_RT_DEBUG),
            &m_LightPass->GetTexture(LIGHTBUFFER_RT_DEBUG),
            &m_LightPass->GetTexture(LIGHTBUFFER_RT_DEBUG),
            &m_LightPass->GetTexture(LIGHTBUFFER_RT_DEBUG),
            &m_LightPass->GetTexture(LIGHTBUFFER_DIRECT_LIGHT),
            &m_GBuffer->GetTexture(GBUFFER_GEOMETRY_MV2D)
        };
        ImGui::Begin("Select render buffer");
        ImGui::ListBox("listbox\n(single select)", &listbox_item_debug, listbox_items, IM_ARRAYSIZE(listbox_items));
        ImGui::End();
        
        commandList->SetShaderResourceView1(DebugTextureParam::texture, 0, *texArray[listbox_item_debug]);

        commandList->Draw(3);

        if (DEBUG_RAYTRACED_ALBEDO <= listbox_item_debug && listbox_item_debug  < DEBUG_RAYTRACED_HIT_T)
           SetRenderTexture(&m_LightPass->GetTexture(LIGHTBUFFER_RT_DEBUG));
        else
            SetRenderTexture(&m_DebugTexturePass->renderTarget.GetTexture(AttachmentPoint::Color0));
    }
    { //Set UAV buffers back to present for denoising
        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->GetTexture(LIGHTBUFFER_DENOISED_INDIRECT_DIFFUSE).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_PRESENT));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->GetTexture(LIGHTBUFFER_DENOISED_INDIRECT_SPECULAR).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_PRESENT));     
    }
    { //Transition GBuffer back to RT
        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_ALBEDO).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_AO_METALLIC_HEIGHT).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_EMISSIVE_SHADER_MODEL).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_LINEAR_DEPTH).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_STANDARD_DEPTH).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_DEPTH_WRITE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_MOTION_VECTOR).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_NORMAL_ROUGHNESS).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_GEOMETRY_NORMAL).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_GBuffer->GetTexture(GBUFFER_GEOMETRY_MV2D).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));

    }
    { //LightPass Transitions
        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->GetTexture(LIGHTBUFFER_DIRECT_LIGHT).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->GetTexture(LIGHTBUFFER_INDIRECT_DIFFUSE).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->GetTexture(LIGHTBUFFER_INDIRECT_SPECULAR).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->GetTexture(LIGHTBUFFER_RT_DEBUG).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->GetTexture(LIGHTBUFFER_ACCUM_BUFFER).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_PRESENT));

    }
    { //CompositionPass barriers
        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_CompositionPass->renderTarget.GetTexture(AttachmentPoint::Color0).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));
    }
    {//Graphics execute
        PIXBeginEvent(graphicsQueue->GetD3D12CommandQueue().Get(), 0, L"Graphics execute");
        graphicsQueue->ExecuteCommandList(commandList);
        PIXEndEvent(graphicsQueue->GetD3D12CommandQueue().Get());
    }  

    {//prev transform for motion vector
        int i = 0;
        for (auto& [transform, mesh] : meshInstances)
            prevTrans[i++] = transform;
    }
    
    cameraCB.prevView = cameraCB.view;
    cameraCB.prevProj = cameraCB.proj;  
}

void DeferredRenderer::OnDlssChanged(const bool& bDlssOn, const NVSDK_NGX_PerfQuality_Value& qualityMode)
{
    ResizeEventArgs arg(m_NativeWidth, m_NativeHeight);
    OnResize(arg);  
}

void DeferredRenderer::OnResize(ResizeEventArgs& e)
{
    //This check happends via the window class
    //if (m_Width == e.Width && m_Height == e.Height) return;

    Application::Get().Flush();

    m_NativeWidth = e.Width;
    m_NativeHeight = e.Height;

    DirectX::XMUINT2 res((unsigned)e.Width, (unsigned)e.Height);

    if (m_DLSS->bDlssOn)
    {
        res = m_DLSS->OnResize(e.Width, e.Height);
    }

    m_Width = res.x;
    m_Height = res.y;

    std::cout << "Native Res: " << e.Width << "x" << e.Height << std::endl;
    std::cout << "Dlss prefered res: " << res.x << "x" << res.y << std::endl;

    m_GBuffer->OnResize(m_Width, m_Height);
    m_LightPass->OnResize(m_Width, m_Height);
    m_CompositionPass->OnResize(m_Width, m_Height);
    m_DebugTexturePass->OnResize(m_Width, m_Height);
    
    //Recreating the denoiser is the way to go (nvidia documentation)
    m_NvidiaDenoiser = std::unique_ptr<NvidiaDenoiser>(new NvidiaDenoiser(m_Width, m_Height, GetDenoiserTextures()));

    m_HDR->OnResize(m_NativeWidth, m_NativeHeight);
}

void DeferredRenderer::Shutdown()
{
}