#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <Shlwapi.h>

#include <Application.h>
#include <Game.h>
#include <GameMode.h>
#include <dxgidebug.h>'
#include <filesystem>
#include <iostream>
#include <Window.h>

typedef GameMode* (CALLBACK* GameEntry)(Game*);

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
        ImGui::TextColored({255.f, 0.f, 0.f, 255.f}, "RELOADING GAME DLL...");
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

class World : public Game
{
public:
    using super = Game;

    World(const std::wstring& name, int width, int height, bool vSync = false)
        :Game(name, width, height, vSync) {};

    virtual ~World() {}


    void LoadGameMode()
    {
        if (this->gameMode)
            this->gameMode->LoadContent();
    }

    void UnLoadGameMode()
    {
        if (this->gameMode)
            this->gameMode->UnloadContent();
    }

    /**
     *  Load content required for the demo.
     */
    virtual bool LoadContent() override
    {
        return true;
    }

    /**
     *  Unload demo specific content that was loaded in LoadContent.
     */
    virtual void UnloadContent() override
    {

    }
protected:
    /**
     *  Update the game logic.
     */
    virtual void OnUpdate(UpdateEventArgs& e) override
    {
        super::OnUpdate(e);

        if (!gameMode) return;

        if (gameMode) gameMode->OnUpdate(e);          
    }

    virtual void OnRender(RenderEventArgs& e) override
    {
        super::OnRender(e);
        OnGui(e);
    }

    void OnGui(RenderEventArgs& e)
    {
        static bool showDemoWindow = false;
        static char gameDLL[20] = {"Sponza"};
        static bool bHotReload = true;
        static float reloadTimer = 0.f;
        static float changesDetectedTimer = 0.f;
        static bool bShowReloadOverlay = false;

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

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Options"))
            {
                bool vSync = m_pWindow->IsVSync();
                if (ImGui::MenuItem("V-Sync", "V", &vSync))
                {
                    m_pWindow->SetVSync(vSync);
                }

                bool fullscreen = m_pWindow->IsFullScreen();
                if (ImGui::MenuItem("Full screen", "Alt+Enter", &fullscreen))
                {
                    m_pWindow->SetFullscreen(fullscreen);
                }

                ImGui::EndMenu();
            }

            char buffer[256];

            ImGui::EndMainMenuBar();
        }

        if (showDemoWindow)
        {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }

        ImGuiInputTextFlags flags;
        flags = ImGuiInputTextFlags_EnterReturnsTrue;

        ImGui::Checkbox("HotReload", &bHotReload);

        ImGui::Text("Game DLL:");
        ImGui::SameLine();

        if (ImGui::InputText("", gameDLL, 20, flags))
        {
            changesDetectedTimer = 1.f;
            UnloadRuntimeGame();
            if (!LoadRuntimeGame(gameDLL))
            {
                ImGui::Text("Game not found!");
            }
        }
        ImGui::SameLine();

        if (ImGui::Button("Reload Dll"))
        {
            changesDetectedTimer = 1.f;
            UnloadRuntimeGame();
            if (!LoadRuntimeGame(gameDLL))
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
            if (CompareFileTime(&runTimeGame.lastWriteTime, &lastWriteTime) != 0)
            {
                changesDetectedTimer = 1.f;
                UnloadRuntimeGame();
                LoadRuntimeGame(gameDLL);

                runTimeGame.lastWriteTime = GetLastWriteTime(dllString);
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
    }
private:
    std::unique_ptr<GameMode> gameMode{nullptr};

    struct RuntimeGame
    {
        HINSTANCE hDLL = NULL;
        GameEntry entry = nullptr;
        FILETIME lastWriteTime{};

        void Reset()
        {
            hDLL = NULL;
            entry = nullptr;
        }
    }runTimeGame;

    bool LoadRuntimeGame(const std::string& name)
    {
        std::string dllString = name + ".dll";
        std::string dllRuntimeString = name + "_Runtime.dll";
        std::string runtimeString = name + "_Runtime";

        std::cout << "Loading runtime game: " << name << std::endl;

        CopyFile(dllString.c_str(), dllRuntimeString.c_str(), false);
        runTimeGame.hDLL = LoadLibrary(runtimeString.c_str());

        if (!runTimeGame.hDLL)
        {
            std::cout << "Dll not found: " << dllString << std::endl;
            return false;
        }

        auto gameEntry = (GameEntry)GetProcAddress(runTimeGame.hDLL, "Entry");

        if (!gameEntry)
        {
            std::cout << "GameEntry not found" << std::endl;
            return false;
        }

        gameMode = std::unique_ptr<GameMode>(gameEntry(this));
        LoadGameMode();
    }

    void UnloadRuntimeGame()
    {
        UnLoadGameMode();
        FreeLibrary(runTimeGame.hDLL);
        runTimeGame.Reset();
        gameMode = nullptr;
    }
};

void ReportLiveObjects()
{
    IDXGIDebug1* dxgiDebug;
    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    dxgiDebug->Release();
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    int retCode = 0;

    WCHAR path[MAX_PATH];

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);
    if (argv)
    {
        for (int i = 0; i < argc; ++i)
        {
            // -wd Specify the Working Directory.
            if (wcscmp(argv[i], L"-wd") == 0)
            {
                wcscpy_s(path, argv[++i]);
                SetCurrentDirectoryW(path);
            }
        }
        LocalFree(argv);
    }

    Application::Create(hInstance);
    {
        auto world = std::shared_ptr<World>(new World(L"Engine", 1280, 720, true));
        Application::Get().Run(world);
    }
    Application::Destroy();

    atexit(&ReportLiveObjects);

    return retCode;
}