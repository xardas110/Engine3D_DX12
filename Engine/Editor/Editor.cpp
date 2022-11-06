#include <EnginePCH.h>
#include <Editor.h>
#include <World.h>
#include <iostream>
#include <Application.h>
#include <Window.h>

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
    if (ImGui::Begin("Example: Simple Overlay", p_open, (corner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
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

Editor::Editor(World* world)
	:m_World(world)
{
}

void Editor::OnGui(RenderEventArgs& e)
{
    if (!m_World) return;

    UpdateGameMenuBar(e);
    UpdateRuntimeGame(e);

    if (showDemoWindow)
    {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }
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

    ImGuiInputTextFlags flags;
    flags = ImGuiInputTextFlags_EnterReturnsTrue;

    ImGui::Checkbox("HotReload", &bHotReload);

    ImGui::Text("Game DLL:");
    ImGui::SameLine();

    if (ImGui::InputText("", gameDLL, 20, flags))
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
