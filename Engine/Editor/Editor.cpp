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

    if (m_UI.bDockFullScreen)
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

    if (m_UI.bDockFullScreen)
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

void Editor::Destroy()
{
}

void Editor::OnUpdate(UpdateEventArgs& e)
{
    ImGui::ShowMetricsWindow();
    UpdateGameMenuBar();
    UpdateWorldHierarchy();
    UpdateMaterialManager();
    UpdateSelectedEntity();
}

MouseButtonEventArgs::MouseButton DecodeMouseButton(KeyCode::Key key)
{
    MouseButtonEventArgs::MouseButton mouseButton = MouseButtonEventArgs::None;
    switch (key)
    {
    case KeyCode::LButton:
    {
        mouseButton = MouseButtonEventArgs::Left;
    }
    break;
    case KeyCode::RButton:
    {
        mouseButton = MouseButtonEventArgs::Right;
    }
    break;
    case KeyCode::MButton:
    {
        mouseButton = MouseButtonEventArgs::Middel;
    }
    break;
    }

    return mouseButton;
}

void Editor::UpdateGameMenuBar()
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
            ImGui::MenuItem("ImGui Demo", nullptr, &m_UI.showDemoWindow);
            ImGui::MenuItem("Game Loader", nullptr, &m_UI.bShowGameLoader);           
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

            ImGui::MenuItem("DockSpace Fullscreen", nullptr, &m_UI.bDockFullScreen);

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Editor::UpdateRuntimeGame(UpdateEventArgs& e)
{
    if (!m_UI.bShowGameLoader) return;

    static bool bHotReload = false;
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

void Editor::UpdateWorldHierarchy()
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

void Editor::UpdateMaterialManager()
{
    auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;
    ImGui::Begin("Material Editor");

    for (auto& [name, materialID] : materialManager.materialData.map)
    {
        auto& material = materialManager.materialData.materials[materialID];

        std::string nameStr = "Material: " + std::string(name.begin(), name.end());
        ImGui::Text(nameStr.c_str());

        ImGui::ColorPicker4("Color", &material.color.x, ImGuiColorEditFlags_Float);
        ImGui::ColorPicker3("Transparency", &material.transparent.x, ImGuiColorEditFlags_Float);
        ImGui::ColorPicker3("Emissive", &material.emissive.x, ImGuiColorEditFlags_Float);
        ImGui::InputFloat("Roughness", &material.roughness);
        ImGui::InputFloat("Metallic", &material.metallic);
        ImGui::Spacing(); ImGui::Spacing();
    }

    ImGui::End();
}

void Editor::UpdateSelectedEntity()
{
    if (selectedEntity == entt::null) return;

    auto& reg = m_World->registry;

    if (!reg.valid(selectedEntity)) return;

    auto& tag = reg.get<TagComponent>(selectedEntity);
    auto& trans = reg.get<TransformComponent>(selectedEntity);

    ImGui::Begin("Selected Entity");
    ImGui::Text("Transform component");
    ImGui::InputFloat3("Position", &trans.pos.m128_f32[0]);
    ImGui::InputFloat3("Scale", &trans.scale.m128_f32[0]);
    ImGui::InputFloat4("Rotation", &trans.rot.m128_f32[0]);
    ImGui::Spacing();
    ImGui::Spacing();

    auto device = Application::Get().GetDevice();
    auto window = m_World->m_pWindow;
    auto& gui = window->m_GUI;

    gui.m_Heap.Resethandle(1);

    if (reg.any_of<MeshComponent>(selectedEntity))
    {
        auto& mesh = reg.get<MeshComponent>(selectedEntity);
        UpdateMeshComponent(mesh);
    }
    if (reg.any_of<StaticMeshComponent>(selectedEntity))
    {
        auto& meshManger = Application::Get().GetAssetManager()->m_MeshManager;
        auto& sm = reg.get<StaticMeshComponent>(selectedEntity);

        for (size_t i = sm.startOffset; i < sm.endOffset; i++)
        {
            auto mesh = MeshInstance(i);
            UpdateMeshComponent(mesh);
        }     
    }

    ImGui::End();
}

void Editor::UpdateMeshComponent(MeshComponent& mesh)
{
    auto device = Application::Get().GetDevice();
    auto window = m_World->m_pWindow;
    auto& gui = window->m_GUI;

    if (!mesh.IsValid()) return;
  
    const std::wstring& wMaterialName = mesh.GetMaterialName();
    std::string materialName = "Material Instance Name: " + std::string(wMaterialName.begin(), wMaterialName.end());
    ImGui::Text(materialName.c_str());
   
    //auto& material = mesh.GetUserMaterial();
  
    const auto& wMatName = mesh.GetUserMaterialName();
    /*
    std::string matName = "User Defined material: " +  std::string(wMatName.begin(), wMatName.end());
    auto materialCopy = material;
    ImGui::Text(matName.c_str());
    ImGui::ColorEdit4("Color", &materialCopy.color.x);
    ImGui::ColorEdit3("Transparency", &materialCopy.transparent.x);
    ImGui::ColorEdit3("Emissive", &materialCopy.emissive.x);
    ImGui::InputFloat("Roughness", &materialCopy.roughness);
    ImGui::InputFloat("Metallic", &materialCopy.metallic);
    */
    ImGui::Text("Material Textures: ");
    ImGui::Spacing();

    for (size_t i = 0; i < MaterialType::NumMaterialTypes; i++)
    {
        const auto texture = mesh.GetTexture((MaterialType::Type)i);

        if (texture)
        {
            UINT lastIndex;
            auto srvCPU = gui.m_Heap.IncrementHandle(lastIndex);
            device->CreateShaderResourceView(texture->GetD3D12Resource().Get(), nullptr, srvCPU);
            auto srvGPU = gui.m_Heap.GetGPUHandle(lastIndex);

            wchar_t name[128] = {};
            UINT size = sizeof(name);
            texture->GetD3D12Resource()->GetPrivateData(WKPDID_D3DDebugObjectNameW, &size, name);

            std::wstring nameWStr = std::wstring(name);
            std::string nameStr = std::string(nameWStr.begin(), nameWStr.end());
            ImGui::Text(nameStr.c_str());
            auto imgSize = ImVec2(128, 128);
            ImGui::Image((ImTextureID)srvGPU.ptr, imgSize);

        }
    }
    
}

void Editor::SelectEntity(entt::entity entity)
{
    selectedEntity = entity;
}
