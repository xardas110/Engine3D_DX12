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
#include <HighResolutionClock.h>

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
    m_DebugTexturePass = std::unique_ptr<DebugTexturePass>(new DebugTexturePass(m_NativeWidth, m_NativeHeight));
    m_HDR = std::unique_ptr<HDR>(new HDR(m_NativeWidth, m_NativeHeight));
    m_Raytracer = std::unique_ptr<Raytracing>(new Raytracing());

    m_Raytracer->Init();

    m_NvidiaDenoiser = std::unique_ptr<NvidiaDenoiser>(new NvidiaDenoiser(m_Width, m_Height, GetDenoiserTextures()));

    m_Skybox = std::unique_ptr<Skybox>(new Skybox(L"Assets/Textures/belfast_sunset_puresky_4k.hdr"));

    m_DLSS->dlssChangeEvent.attach(&DeferredRenderer::OnDlssChanged, this);
}

void DeferredRenderer::LoadContent()
{
    auto copyQueue = Application::Get().GetCommandQueue();

    auto cl = copyQueue->GetCommandList();

    cl->LoadTextureFromFile(scrambling, L"Assets/Textures/BlueNoise/scrambling_ranking_128x128_2d_1spp.png");
    cl->LoadTextureFromFile(sobol, L"Assets/Textures/BlueNoise/sobol_256_4d.png");

    auto fVal = copyQueue->ExecuteCommandList(cl);
    copyQueue->WaitForFenceValue(fVal);

    auto device = Application::Get().GetDevice();

    D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
    desc.Format = DXGI_FORMAT_R8G8B8A8_UINT;
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Texture2D.MipLevels = 1;
    desc.Texture2D.MostDetailedMip = 0;

    device->CreateShaderResourceView(
        scrambling.GetD3D12Resource().Get(), &desc,
        heap.SetHandle(DENOISER_TEX_SCRAMBLING));

    device->CreateShaderResourceView(
        sobol.GetD3D12Resource().Get(), &desc,
        heap.SetHandle(DENOISER_TEX_SOBOL));
     
}

DeferredRenderer::~DeferredRenderer()
{
    m_DLSS->dlssChangeEvent.detach(&DeferredRenderer::OnDlssChanged, this);
    Application::Get().m_DLSS = std::move(m_DLSS);
}

std::vector<MeshInstanceWrapper> DeferredRenderer::GetMeshInstances(entt::registry& registry, std::vector<MeshInstanceWrapper>* pointLights)
{
    auto& meshManager = Application::Get().GetAssetManager()->m_MeshManager;
    std::vector<MeshInstanceWrapper> instances;
    {
        auto& view = registry.view<TransformComponent, MeshComponent>();
        for (auto [entity, transform, mesh] : view.each())
        {
            MeshInstanceWrapper wrap(transform, mesh);

            if (mesh.IsPointlight())
            { 
                if (pointLights)
                    pointLights->emplace_back(wrap);
            }

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

                if (MeshInstance(i).IsPointlight())
                {
                    if (pointLights)
                        pointLights->emplace_back(wrap);
                }

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
        m_LightPass->GetTexture(LIGHTBUFFER_DENOISED_INDIRECT_SPECULAR),
        m_LightPass->GetTexture(LIGHTBUFFER_SHADOW_DATA),
        m_LightPass->GetTexture(LIGHTBUFFER_DENOISED_SHADOW)
    );
    return dt;
}

void DeferredRenderer::Render(Window& window, const RenderEventArgs& e)
{
    if (!window.m_pGame.lock()) return;
 
    auto clockNow = std::chrono::high_resolution_clock::now();

    m_HDR->UpdateGUI();
    m_DLSS->OnGUI();
    m_NvidiaDenoiser->OnGUI();

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

    std::vector<MeshInstanceWrapper> pointLights;
    auto meshInstances = GetMeshInstances(game->registry, &pointLights);

    prevTrans.resize(meshInstances.size());

    auto* camera = game->GetRenderCamera();

    XMFLOAT2 jitter = camera->jitter;
    jitter.x /= float(m_Width);
    jitter.y /= float(m_Height);

    ObjectCB objectCB; //todo move to cam
    objectCB.view = camera->get_ViewMatrix();
    objectCB.proj = camera->get_ProjectionMatrix();
    objectCB.invView = XMMatrixInverse(nullptr, objectCB.view);
    objectCB.invProj = XMMatrixInverse(nullptr, objectCB.proj);

    XMMATRIX jitterMat = XMMatrixIdentity();
    jitterMat.r[3].m128_f32[0] = jitter.x;
    jitterMat.r[3].m128_f32[1] = jitter.y;

    cameraCB.view = camera->get_ViewMatrix();
    cameraCB.proj = camera->get_ProjectionMatrix();
    cameraCB.invView = XMMatrixInverse(nullptr, cameraCB.view);
    cameraCB.invProj = XMMatrixInverse(nullptr, cameraCB.proj);
    cameraCB.viewProj = cameraCB.view * cameraCB.proj;
    cameraCB.invViewProj = XMMatrixInverse(nullptr, cameraCB.viewProj);
    cameraCB.resolution = { (float)m_Width, (float)m_Height };
    cameraCB.zNear = camera->GetNear();
    cameraCB.zFar = camera->GetFar();
    cameraCB.eyeToPixelConeSpreadAngle = atanf((2.0f * tanf(camera->get_FoV() * 0.5f)) / (float)m_Height);
    cameraCB.jitter = jitter;

    RaytracingDataCB rtData;
    rtData.frameNumber = Application::GetFrameCount();
    rtData.debugSettings = listbox_item_debug;
    auto& hitParams = m_NvidiaDenoiser->denoiserSettings.settings.hitDistanceParameters;
    rtData.hitParams = { hitParams.A, hitParams.B, hitParams.C, hitParams.D};
    rtData.frameIndex = window.m_CurrentBackBufferIndex;

    static int numBounces = 3;
    static float bounceAmbientStrength = 0.5f;

    ImGui::Begin("Raytracing Settings");
    ImGui::SliderInt("Num Bounces", &numBounces, 0, 10);
    ImGui::SliderFloat("1 bounce ambient strength", &bounceAmbientStrength, 0.f, 1.f);
    rtData.numBounces = numBounces;
    rtData.oneBounceAmbientStrength = bounceAmbientStrength;
    ImGui::End();

    XMStoreFloat3(&cameraCB.pos, camera->get_Translation());

    auto& directionalLight = game->m_DirectionalLight;
    directionalLight.UpdateUI();

    {//Camera Settings
        ImGui::Begin("Camera");
        ImGui::Checkbox("Jitter", &game->m_Camera.bJitter);
        ImGui::End();
    }

    LightDataCB lightDataCB{};
    {//Lights

        //Populate directional light first, to make sure it will always be importance sampled
        lightDataCB.lights[0].type = LIGHT_DIRECTIONAL;
        XMStoreFloat3(&lightDataCB.lights[0].intensity, directionalLight.GetData().color * directionalLight.GetData().color.m128_f32[3]);
        XMStoreFloat3(&lightDataCB.lights[0].pos, -directionalLight.GetData().direction);
        lightDataCB.numLights++;

        for (size_t i = 0; i < pointLights.size(); i++)
        {
            if (i + 1 >= MAX_LIGHTS) break;

            auto& ptLight = pointLights[i];
            lightDataCB.numLights++;

            auto& mat = ptLight.instance.GetUserMaterial();

            lightDataCB.lights[i+1].type = LIGHT_POINT;
            lightDataCB.lights[i+1].intensity = XMFLOAT3(20.f, 20.f, 20.f);
            XMStoreFloat3(&lightDataCB.lights[i+1].pos, ptLight.trans.pos);
        }
    }

    {// Clear the render targets.
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

       // ImGui::Begin("RtStructure");
       // ImGui::Checkbox("Build pr. frame", &m_Raytracer->bUpdate);
       // ImGui::End();

    } 
    /*
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

            objectCB.objRotQuat = transform.rot;
            globalMeshInfo[mesh.id].objRot = transform.rot;

            commandList->SetGraphicsDynamicConstantBuffer(DepthPrePassParam::ObjectCB, objectCB);

            if (mesh.HasOpacity()) continue;

            assetManager->m_MeshManager.meshData.meshes[meshInstance.meshIds[mesh.id]].mesh.Draw(*commandList);
        }
        PIXEndEvent(gfxCommandList.Get());
    }
    */
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

            if (mesh.IsPointlight())
            {
                i++;
                continue;
            }

            objectCB.model = transform.GetTransform();

            objectCB.mvp = objectCB.model * objectCB.view * objectCB.proj * jitterMat;
            objectCB.prevMVP = prevTrans[i].GetTransform() * cameraCB.prevView * cameraCB.prevProj * jitterMat;

            objectCB.invTransposeMvp = XMMatrixInverse(nullptr, XMMatrixTranspose(objectCB.mvp));
            objectCB.meshId = mesh.id;

            objectCB.prevModel = prevTrans[i].GetTransform();
            objectCB.transposeInverseModel = (XMMatrixInverse(nullptr, XMMatrixTranspose(objectCB.model)));
            objectCB.objRotQuat = transform.rot;

            globalMeshInfo[mesh.id].objRot = transform.rot;

            auto matInstanceID = globalMeshInfo[mesh.id].materialInstanceID;
            objectCB.materialGPUID = matInstanceID;

            commandList->SetGraphicsDynamicConstantBuffer(GBufferParam::ObjectCB, objectCB);
          
            assetManager->m_MeshManager.meshData.meshes[meshInstance.meshIds[mesh.id]].mesh.Draw(*commandList);

            i++;
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
        commandList->SetGraphicsDynamicConstantBuffer(LightPassParam::DirectionalLightCB, directionalLight.GetData());
        commandList->SetGraphicsDynamicConstantBuffer(LightPassParam::RaytracingDataCB, rtData);
        commandList->SetGraphicsDynamicConstantBuffer(LightPassParam::LightDataCB, lightDataCB);

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_Skybox->heap.heap.Get());
        gfxCommandList->SetGraphicsRootDescriptorTable(LightPassParam::Cubemap, m_Skybox->GetSRVView());

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, heap.heap.Get());
        gfxCommandList->SetGraphicsRootDescriptorTable(LightPassParam::BlueNoiseTextures, heap.GetHandleAtStart());

        commandList->Draw(3);

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->GetTexture(LIGHTBUFFER_DIRECT_LIGHT).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->GetTexture(LIGHTBUFFER_SHADOW_DATA).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->renderTarget.GetTexture(AttachmentPoint::Color4).GetD3D12Resource().Get(),
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
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

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
                dt.inShadowData.GetD3D12Resource().Get(),
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

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                dt.outShadow.GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

        m_NvidiaDenoiser->RenderFrame(*commandList, cameraCB, *camera, window.m_CurrentBackBufferIndex, m_Width, m_Height);

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_GBuffer->m_SRVHeap.heap.Get());

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                dt.inMV.GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
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
                dt.inShadowData.GetD3D12Resource().Get(),
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

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                dt.outShadow.GetD3D12Resource().Get(),
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

        commandList->SetGraphicsDynamicConstantBuffer(CompositionPassParam::RaytracingDataCB, rtData);
        commandList->SetGraphicsDynamicConstantBuffer(CompositionPassParam::CameraCB, cameraCB);
        commandList->SetGraphicsDynamicConstantBuffer(CompositionPassParam::TonemapCB, m_HDR->GetTonemapCB());

        commandList->Draw(3);

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_CompositionPass->renderTarget.GetTexture(AttachmentPoint::Color0).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        PIXEndEvent(gfxCommandList.Get());
    } 
    {//Histogram pass
        PIXBeginEvent(gfxCommandList.Get(), 0, L"Histogram Pass");

        UINT histoClear[4] = { 0, 0, 0, 0 };

        gfxCommandList->ClearUnorderedAccessViewUint(
            m_HDR->histogramClearHeap.heap->GetGPUDescriptorHandleForHeapStart(),
            m_HDR->histogramClearHeap.heap->GetCPUDescriptorHandleForHeapStart(),
            m_HDR->histogram.GetD3D12Resource().Get(), histoClear, 0, nullptr);

        commandList->SetPipelineState(m_HDR->histogramPSO);
        commandList->SetComputeRootSignature(m_HDR->histogramRT);

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_CompositionPass->renderTarget.GetTexture(AttachmentPoint::Color0).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_CompositionPass->heap.heap.Get());
        gfxCommandList->SetComputeRootDescriptorTable(HistogramParam::SourceTex, m_CompositionPass->heap.GetGPUHandle(0));

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_HDR->histogramHeap.heap.Get());
        gfxCommandList->SetComputeRootDescriptorTable(HistogramParam::Histogram, m_HDR->histogramHeap.GetGPUHandle(0));
   
        auto tonemapCB = m_HDR->GetTonemapCB();
        tonemapCB.logLuminanceScale = 1.0f / (tonemapCB.maxLogLuminamce - tonemapCB.minLogLuminance);
        tonemapCB.logLuminanceBias = -tonemapCB.minLogLuminance * tonemapCB.logLuminanceScale;

        tonemapCB.viewOrigin = { 0.f, 0.f };
        tonemapCB.viewSize = { (float)m_Width, (float)m_Height };
        tonemapCB.sourceSlice = 0;
        
        commandList->SetComputeDynamicConstantBuffer(HistogramParam::TonemapCB, tonemapCB);

        commandList->Dispatch((tonemapCB.viewSize.x + 15) / HISTROGRAM_GROUP_X , (tonemapCB.viewSize.y + 15) / HISTROGRAM_GROUP_Y, 1);

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_CompositionPass->renderTarget.GetTexture(AttachmentPoint::Color0).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

        PIXEndEvent(gfxCommandList.Get());
    }
    { //Exposure pass
        PIXBeginEvent(gfxCommandList.Get(), 0, L"Exposure Pass");      
        commandList->SetPipelineState(m_HDR->exposurePSO);     
        commandList->SetComputeRootSignature(m_HDR->exposureRT);

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_HDR->exposureHeap.heap.Get());
        gfxCommandList->SetComputeRootDescriptorTable(ExposureParam::ExposureTex, m_HDR->exposureHeap.GetGPUHandle(0));

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_HDR->histogramHeap.heap.Get());
        gfxCommandList->SetComputeRootDescriptorTable(ExposureParam::Histogram, m_HDR->histogramHeap.GetGPUHandle(0));


        auto tonemapCB = m_HDR->GetTonemapCB();
        auto& eyeAdapt = m_HDR->eyeAdaption;

        tonemapCB.logLuminanceScale = tonemapCB.maxLogLuminamce - tonemapCB.minLogLuminance;
        tonemapCB.logLuminanceBias = tonemapCB.minLogLuminance;
        tonemapCB.histogramLowPercentile = std::min(0.99f, std::max(0.f, eyeAdapt.lowPercentile));
        tonemapCB.histogramHighPercentile = std::min(1.f, std::max(tonemapCB.histogramLowPercentile, eyeAdapt.highPercentile));
        tonemapCB.eyeAdaptationSpeedUp = eyeAdapt.eyeAdaptionSpeedUp;
        tonemapCB.eyeAdaptationSpeedDown = eyeAdapt.eyeAdaptionSpeedDown;
        tonemapCB.minAdaptedLuminance = eyeAdapt.minAdaptedLuminance;
        tonemapCB.maxAdaptedLuminance = eyeAdapt.maxAdaptedLuminance;
        tonemapCB.frameTime = e.ElapsedTime;
        
        commandList->SetComputeDynamicConstantBuffer(ExposureParam::TonemapCB, tonemapCB);

        commandList->Dispatch(1,1,1);
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
                &m_GBuffer->GetTexture(GBUFFER_STANDARD_DEPTH),
                &m_HDR->exposureTex
            );

            dlssTextures.linearDepth = &m_GBuffer->GetTexture(GBUFFER_LINEAR_DEPTH);
            dlssTextures.motionVectors3D = &m_GBuffer->GetTexture(GBUFFER_MOTION_VECTOR);

            m_DLSS->EvaluateSuperSampling(commandList.get(), dlssTextures, m_NativeWidth, m_NativeHeight, *camera);

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

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_HDR->exposureHeap.heap.Get());
        gfxCommandList->SetGraphicsRootDescriptorTable(HDRParam::ExposureTex, m_HDR->exposureHeap.GetGPUHandle(0));

        auto tonemapCB = m_HDR->GetTonemapCB();
        const auto& eyeAdapt = m_HDR->eyeAdaption;

        tonemapCB.exposureScale = ::exp2f(eyeAdapt.exposureBias);
        tonemapCB.whitePointInvSquared = 1.f / powf(eyeAdapt.whitePoint, 2.f);
        tonemapCB.minAdaptedLuminance = eyeAdapt.minAdaptedLuminance;
        tonemapCB.maxAdaptedLuminance = eyeAdapt.maxAdaptedLuminance;
        tonemapCB.sourceSlice = 0;

        commandList->SetGraphicsDynamicConstantBuffer(HDRParam::TonemapCB, tonemapCB);

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
            "GBuffer MV2D",
            "LightBuffer Translucent",
            "LightBuffer Unfiltered Shadow",
            "LightBuffer Denoised Shadow"
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
            &m_GBuffer->GetTexture(GBUFFER_GEOMETRY_MV2D),
            &m_LightPass->renderTarget.GetTexture(AttachmentPoint::Color4),
            &m_LightPass->GetTexture(LIGHTBUFFER_SHADOW_DATA),
            &m_LightPass->denoisedShadow
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

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->GetTexture(LIGHTBUFFER_DENOISED_SHADOW).GetD3D12Resource().Get(),
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
                m_LightPass->GetTexture(LIGHTBUFFER_SHADOW_DATA).GetD3D12Resource().Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET));

        gfxCommandList->ResourceBarrier(1,
            &CD3DX12_RESOURCE_BARRIER::Transition(
                m_LightPass->renderTarget.GetTexture(AttachmentPoint::Color4).GetD3D12Resource().Get(),
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

    auto clockEnd = std::chrono::high_resolution_clock::now();

    ImGui::Begin("RenderStats");
    std::string elapsedTime = "CPU MS: " + std::to_string((clockEnd - clockNow).count() * 1e-6);
    ImGui::Text(elapsedTime.c_str());
    ImGui::End();
}

void DeferredRenderer::OnDlssChanged(const bool& bDlssOn, const NVSDK_NGX_PerfQuality_Value& qualityMode)
{
    ResizeEventArgs arg(m_NativeWidth, m_NativeHeight);
    OnResize(arg);  
}

void DeferredRenderer::OnResize(ResizeEventArgs& e)
{
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
    m_DebugTexturePass->OnResize(m_NativeWidth, m_NativeHeight);
    
    //Recreate the denoiser(nvidia documentation)
    m_NvidiaDenoiser = std::unique_ptr<NvidiaDenoiser>(new NvidiaDenoiser(m_Width, m_Height, GetDenoiserTextures()));

    m_HDR->OnResize(m_NativeWidth, m_NativeHeight);
}

void DeferredRenderer::Shutdown()
{
}