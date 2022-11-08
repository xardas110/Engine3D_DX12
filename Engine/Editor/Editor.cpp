#include <EnginePCH.h>
#include <Editor.h>
#include <World.h>
#include <iostream>
#include <Application.h>
#include <Window.h>
#include <Components.h>
#include <Entity.h>
#include <imgui_impl_dx12.h>

static ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow
| ImGuiTreeNodeFlags_OpenOnDoubleClick;

static ImGuiTreeNodeFlags leafFlags = nodeFlags
| ImGuiTreeNodeFlags_Leaf
| ImGuiTreeNodeFlags_NoTreePushOnOpen;

static ImGuiTreeNodeFlags selectedFlag = ImGuiTreeNodeFlags_Selected;

FILETIME GetLastWriteTime(const std::string& file)
{
    FILETIME lastWriteTime = FILETIME{};

    WIN32_FIND_DATA findData;
    HANDLE findHandle = FindFirstFileA(file.c_str(), &findData);

    if (findHandle != INVALID_HANDLE_VALUE)
    {
        lastWriteTime = findData.ftLastWriteTime;
        FindClose(findHandle);
    }
    else
    {
        std::cout << "File handle not found" << std::endl;
    }
    return lastWriteTime;
}

static void ShowReloadOverlay(bool* p_open)
{
    const float DISTANCE = 20.0f;
    static int corner = 0;
    ImVec2 window_pos = ImVec2((corner & 1) ? ImGui::GetIO().DisplaySize.x - DISTANCE : DISTANCE, (corner & 2) ? ImGui::GetIO().DisplaySize.y - DISTANCE : DISTANCE);
    ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
    if (corner != -1)
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowBgAlpha(0.3f); // Transparent background
    if (ImGui::Begin("Overlay", p_open, (corner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMouseInputs))
    {
        ImGui::TextColored({ 255.f, 0.f, 0.f, 255.f }, "RELOADING GAME DLL...");
        ImGui::Separator();

        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::MenuItem("Custom", NULL, corner == -1)) corner = -1;
            if (ImGui::MenuItem("Top-left", NULL, corner == 0)) corner = 0;
            if (ImGui::MenuItem("Top-right", NULL, corner == 1)) corner = 1;
            if (ImGui::MenuItem("Bottom-left", NULL, corner == 2)) corner = 2;
            if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
            if (p_open && ImGui::MenuItem("Close")) *p_open = false;
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

void Editor::ShowDockSpace(bool* p_open)
{
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

    if (bDockFullScreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::Begin("DockSpace", p_open, window_flags);

    if (bDockFullScreen)
        ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    ImGui::End();
}

Editor::Editor(World* world)
	:m_World(world)
{

}

void Editor::OnGui(RenderEventArgs& e)
{
    if (!m_World) return;

    ShowDockSpace(&bUseDocking);
    ImGui::ShowMetricsWindow();

    UpdateGameMenuBar(e);
    UpdateWorldHierarchy(e);
    UpdateSelectedEntity(e);

    //Beware! Might need to run last
    UpdateRuntimeGame(e);
  
    if (showDemoWindow)
    {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }
}

void Editor::OnViewportRender(const Texture& sceneTexture, ID3D12DescriptorHeap* heap)
{
    auto device = Application::Get().GetDevice();
    UINT handle_increment = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    D3D12_CPU_DESCRIPTOR_HANDLE my_texture_srv_cpu_handle = heap->GetCPUDescriptorHandleForHeapStart();
    my_texture_srv_cpu_handle.ptr += (handle_increment);
    D3D12_GPU_DESCRIPTOR_HANDLE my_texture_srv_gpu_handle = heap->GetGPUDescriptorHandleForHeapStart();
    my_texture_srv_gpu_handle.ptr += (handle_increment);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    device->CreateShaderResourceView(sceneTexture.GetD3D12Resource().Get(), &srvDesc, my_texture_srv_cpu_handle);

    ImGui::Begin("ViewPort", 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);   
    ImGui::Image((ImTextureID)my_texture_srv_gpu_handle.ptr, ImGui::GetWindowSize());
    ImGui::End();
}

void Editor::UpdateGameMenuBar(RenderEventArgs& e)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit", "Esc"))
            {
                Application::Get().Quit();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::MenuItem("ImGui Demo", nullptr, &showDemoWindow);
            ImGui::MenuItem("Game Loader", nullptr, &bShowGameLoader);
            ImGui::MenuItem("DockSpace Fullscreen", nullptr, &bDockFullScreen);            
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Options"))
        {
            bool vSync = m_World->m_pWindow->IsVSync();
            if (ImGui::MenuItem("V-Sync", "V", &vSync))
            {
                m_World->m_pWindow->SetVSync(vSync);
            }

            bool fullscreen = m_World->m_pWindow->IsFullScreen();
            if (ImGui::MenuItem("Full screen", "Alt+Enter", &fullscreen))
            {
                m_World->m_pWindow->SetFullscreen(fullscreen);
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Editor::UpdateRuntimeGame(RenderEventArgs& e)
{
    if (!bShowGameLoader) return;

    static bool bHotReload = true;
    static float reloadTimer = 0.f;
    static float changesDetectedTimer = 0.f;
    static bool bShowReloadOverlay = false;

    ImGui::Begin("Game Loader");
    ImGui::Checkbox("HotReload", &bHotReload);
    ImGui::Text("Game DLL:");
    ImGui::SameLine();

    if (ImGui::InputText(":", gameDLL, 20))
    {
        changesDetectedTimer = 1.f;
        m_World->UnloadRuntimeGame();
        if (!m_World->LoadRuntimeGame(gameDLL))
        {
            ImGui::Text("Game not found!");
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Reload"))
    {
        changesDetectedTimer = 1.f;
        m_World->UnloadRuntimeGame();
        if (!m_World->LoadRuntimeGame(gameDLL))
        {
            ImGui::Text("Game not found!");
        }
    }

    reloadTimer -= (float)e.ElapsedTime;
    changesDetectedTimer -= (float)e.ElapsedTime;

    if (bHotReload && reloadTimer < 0.f)
    {
        reloadTimer = 0.5f;

        std::string dllString = std::string(gameDLL) + ".dll";

        FILETIME lastWriteTime = GetLastWriteTime(dllString);
        if (CompareFileTime(&m_World->runTimeGame.lastWriteTime, &lastWriteTime) != 0)
        {
            changesDetectedTimer = 1.f;
            m_World->UnloadRuntimeGame();
            m_World->LoadRuntimeGame(gameDLL);

            m_World->runTimeGame.lastWriteTime = GetLastWriteTime(dllString);
        }
    }
    if (changesDetectedTimer > 0)
    {
        bShowReloadOverlay = true;
        ShowReloadOverlay(&bShowReloadOverlay);
    }
    else
    {
        bShowReloadOverlay = false;
    }
  
    ImGui::End();
}

void Editor::UpdateWorldHierarchy(RenderEventArgs& e)
{
    ImGui::Begin("World Hierarchy");   
    auto view = m_World->registry.view<TagComponent, RelationComponent>();
    for (auto [entity, tag, relation] : view.each())
    {     
        if (relation.HasParent()) continue;

        UpdateSceneGraph(entity, tag.GetTag(), relation);
    }             
   ImGui::End();
}

void Editor::UpdateSceneGraph(entt::entity entity, const std::string& tag, RelationComponent& relation)
{
    bool bNodeOpen = false;

    if (relation.HasChildren())
    {
        bNodeOpen = ImGui::TreeNodeEx(
            (void*)(intptr_t)entity,
            entity == selectedEntity ? nodeFlags | selectedFlag : nodeFlags,
            std::string(tag + " -> Id: " + std::to_string((std::uint32_t)entity)).c_str(),
            static_cast<uint32_t>(entity)
        );
    }
    else
    {
        ImGui::TreeNodeEx(
            (void*)(intptr_t)entity,
            entity == selectedEntity ? leafFlags | selectedFlag : leafFlags,
            std::string(tag + " -> Id: " + std::to_string((std::uint32_t)entity)).c_str(),
            static_cast<uint32_t>(entity)
        );
    }

    if (ImGui::IsItemClicked())
    {
        SelectEntity(entity);
    }

    if (bNodeOpen)
    {
        for (auto child : relation.GetChildren())
        {
            auto& childTag = m_World->registry.get<TagComponent>(child);
            auto& childRelation = m_World->registry.get<RelationComponent>(child);
            UpdateSceneGraph(child, childTag.GetTag(), childRelation);
        }
        ImGui::TreePop();
    }
}

void Editor::UpdateSelectedEntity(RenderEventArgs& e)
{
    if (selectedEntity == entt::null) return;

    auto& reg = m_World->registry;

    if (!reg.valid(selectedEntity)) return;

    auto& tag = reg.get<TagComponent>(selectedEntity);
    auto& trans = reg.get<TransformComponent>(selectedEntity);

    ImGui::Begin(std::string("Selected Entity: " + tag.GetTag() + " -> id: " + std::to_string((std::uint32_t)selectedEntity)).c_str());
    ImGui::InputFloat3("Position", &trans.pos.m128_f32[0]);
    ImGui::InputFloat3("Scale", &trans.scale.m128_f32[0]);
    ImGui::InputFloat4("Rotation", &trans.rot.m128_f32[0]);
    ImGui::End();
}

void Editor::SelectEntity(entt::entity entity)
{
    selectedEntity = entity;
}
