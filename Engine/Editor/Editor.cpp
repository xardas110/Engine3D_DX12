#include <EnginePCH.h>
#include <Editor.h>
#include <World.h>
#include <iostream>
#include <Application.h>
#include <Window.h>
#include <Components.h>
#include <Entity.h>
#include <imgui_impl_dx12.h>

#ifdef DEBUG_EDITOR
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
    m_Gui.Initialize(world->m_pWindow);
}

void Editor::Destroy()
{
    m_Gui.Destroy();
}

void Editor::OnUpdate(UpdateEventArgs& e)
{
    auto mainWindow = m_World->m_pWindow;

    m_Gui.NewFrame();

    m_UpdateClock.Tick();
    UpdateEventArgs updateEventArgs(m_UpdateClock.GetDeltaSeconds(), m_UpdateClock.GetTotalSeconds(), e.FrameNumber);  

    RenderGui(updateEventArgs);

    PollWindowInputs(mainWindow);
    mainWindow->OnUpdate(updateEventArgs);

}

void Editor::OnRender(RenderEventArgs& e)
{
    auto mainWindow = m_World->m_pWindow;

    m_RenderClock.Tick();
    RenderEventArgs renderEventArgs(m_RenderClock.GetDeltaSeconds(), m_RenderClock.GetTotalSeconds(), e.FrameNumber);

    mainWindow->OnRender(renderEventArgs);
    OnViewportRender(mainWindow);

    mainWindow->Present(Texture(), m_Gui);

    m_Gui.ResetHeapHandle();
}

void Editor::OnViewportRender(std::shared_ptr<Window> window)
{
    auto device = Application::Get().GetDevice();

    D3D12_CPU_DESCRIPTOR_HANDLE my_texture_srv_cpu_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE my_texture_srv_gpu_handle;
    
    m_Gui.GetNextHeapHandle(my_texture_srv_cpu_handle, my_texture_srv_gpu_handle);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    device->CreateShaderResourceView(window->GetDeferredRendererFinalTexture().GetD3D12Resource().Get(), &srvDesc, my_texture_srv_cpu_handle);

    ImGui::Begin(std::string(std::string(window->GetWindowName().begin(), window->GetWindowName().end()) + " Viewport").c_str(), 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
    ImGui::Image((ImTextureID)my_texture_srv_gpu_handle.ptr, ImGui::GetWindowSize());
    ImGui::End();
}

void Editor::OnResize(ResizeEventArgs& e, std::shared_ptr<Window>& window)
{
    //Resizes swapchain relative to the window
    //This will only resize the swapchain and the renderwindow
    //The game will be resized by the imgui viewport
    window->OnResize(e);
}

void Editor::RenderGui(UpdateEventArgs& e)
{
    if (!m_World) return;

    ShowDockSpace(&m_UI.bUseDocking);
    ImGui::ShowMetricsWindow();

    UpdateGameMenuBar();
    UpdateWorldHierarchy();
    UpdateSelectedEntity();

    //Beware! Might need to run last
    UpdateRuntimeGame(e);

    if (m_UI.showDemoWindow)
    {
        ImGui::ShowDemoWindow(&m_UI.showDemoWindow);
    }
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

void Editor::PollWindowInputs(std::shared_ptr<Window> window)
{
    ViewportData& data = m_ViewportDataMap[window->GetWindowName()];
    ViewportData& previousData = m_PreviousViewportDataMap[window->GetWindowName()];

    std::string title = 
        std::string(std::string(window->GetWindowName().begin(),      
        window->GetWindowName().end()) + " Viewport");

    ImGui::Begin(title.c_str(), 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);

    POINT p;
    if (GetCursorPos(&p))
    {
        data.mouseGlobalPosX = p.x;
        data.mouseGlobalPosY = p.y;
    }

    ResizeEventArgs e(ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
    if (data.width != e.Width || data.height != e.Height)
    { 
        data.width = e.Width; 
        data.height = e.Height;

        window->m_DeferredRenderer.OnResize(e);
        if (auto pGame = window->m_pGame.lock())
        {
            pGame->OnResize(e);
        }
    }

    data.bFocused = ImGui::IsWindowFocused();
    data.bHovered = ImGui::IsWindowHovered();

    data.mousePosX = data.mouseGlobalPosX - ImGui::GetCursorScreenPos().x;
    data.mousePosY = data.mouseGlobalPosY - ImGui::GetCursorScreenPos().y;

    if (data.bHovered)
    { 
        data.bShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        data.bControl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        data.bAlt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

        for (int i = 0; i < KeyCode::Size; i++)
        {
            KeyCode::Key keyCode = KeyCode::Key(i);
            if (GetAsyncKeyState(keyCode))
            {
                if (!data.keys[keyCode])
                { 
                    if (keyCode >= KeyCode::None && keyCode <= KeyCode::XButton2)
                    {
                        MouseButtonEventArgs mouseEventArgs(
                            DecodeMouseButton(keyCode), 
                            MouseButtonEventArgs::Pressed, data.keys[KeyCode::Left],
                            data.keys[KeyCode::MButton],
                            data.keys[KeyCode::Right],
                            data.bControl, data.bShift, data.mousePosX, data.mousePosY);
                        window->OnMouseButtonPressed(mouseEventArgs);
                    }
                    else
                    { 
                        KeyEventArgs keyEventArgs(keyCode, 0, KeyEventArgs::Pressed, data.bControl, data.bShift, data.bAlt);
                        window->OnKeyPressed(keyEventArgs);
                    }
                }

                data.keys[keyCode] = true;
            }
            else
            {
                if (data.keys[keyCode])
                {
                    if (keyCode >= KeyCode::None && keyCode <= KeyCode::XButton2)
                    {
                        MouseButtonEventArgs mouseEventArgs(
                            DecodeMouseButton(keyCode),
                            MouseButtonEventArgs::Released, data.keys[KeyCode::Left],
                            data.keys[KeyCode::MButton],
                            data.keys[KeyCode::Right],
                            data.bControl, data.bShift, data.mousePosX, data.mousePosY);
                        window->OnMouseButtonReleased(mouseEventArgs);
                    }
                    else
                    { 
                        KeyEventArgs keyEventArgs(keyCode, 0, KeyEventArgs::Released, data.bControl, data.bShift, data.bAlt);
                        window->OnKeyReleased(keyEventArgs);
                    }
                }

                data.keys[keyCode] = false;
            }
        }

        if (!Cmp(previousData.mousePosX, data.mousePosX) || !Cmp(previousData.mousePosY, data.mousePosY))
        { 
            MouseMotionEventArgs mouseMotion(data.keys[KeyCode::LButton], 
                data.keys[KeyCode::MButton], data.keys[KeyCode::RButton], 
                data.bControl, data.bShift, data.mousePosX, data.mousePosY);
            window->OnMouseMoved(mouseMotion);
        }     
    }
    
    //Release all buttons if mouse is outside viewport
    if (!Cmp(data.bHovered, previousData.bHovered))
    {
        for (int i = 0; i < KeyCode::Size; i++)
        {
            KeyCode::Key keyCode = KeyCode::Key(i);

            if (data.keys[keyCode])
            {           
                MouseButtonEventArgs mouseEventArgs(
                    DecodeMouseButton(keyCode),
                    MouseButtonEventArgs::Released, data.keys[KeyCode::Left],
                    data.keys[KeyCode::MButton],
                    data.keys[KeyCode::Right],
                    data.bControl, data.bShift, data.mousePosX, data.mousePosY);
                window->OnMouseButtonReleased(mouseEventArgs);

                KeyEventArgs keyEventArgs(keyCode, 0, KeyEventArgs::Released, data.bControl, data.bShift, data.bAlt);
                window->OnKeyReleased(keyEventArgs);

                data.keys[keyCode] = false;
            }
        }
    }   
    ImGui::End();

    previousData = data;
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

void Editor::UpdateSelectedEntity()
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

#endif // DEBUG_EDITOR
