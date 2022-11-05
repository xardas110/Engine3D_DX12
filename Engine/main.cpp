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

typedef GameMode* (CALLBACK* GameEntry)(Game*);

class GameWrapper : public Game
{
public:
    using super = Game;

    GameWrapper(const std::wstring& name, int width, int height, bool vSync = false)
        :Game(name, width, height, vSync) {};

    virtual ~GameWrapper() {}

    void SetGameMode(std::shared_ptr<GameMode> gameMode)
    {
        this->gameMode = gameMode;
    }

    /**
     *  Load content required for the demo.
     */
    virtual bool LoadContent() override
    {
        return gameMode->LoadContent();
    }

    /**
     *  Unload demo specific content that was loaded in LoadContent.
     */
    virtual void UnloadContent() override
    {
        gameMode->UnloadContent();
    }
protected:
    /**
     *  Update the game logic.
     */
    virtual void OnUpdate(UpdateEventArgs& e) override
    {
        super::OnUpdate(e);
        gameMode->OnUpdate(e);
    }
private:
    std::shared_ptr<GameMode> gameMode;
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
        HINSTANCE hDLL;
        while (true)
        {
            hDLL = LoadLibrary("Sponza");
            if (NULL != hDLL)
            {
                printf("lib found\n");

                auto gameEntry = (GameEntry)GetProcAddress(hDLL, "Entry");
                if (NULL != gameEntry)
                {
                    printf("func found\n");
                    auto game = new GameWrapper(L"Sponza Test Scene", 1280, 720, true);
                    auto gm = std::shared_ptr<GameMode>(gameEntry(game));
                    game->SetGameMode(gm);

                    Application::Get().Run(std::shared_ptr<Game>(game));
                }
                else
                {
                    printf("func not found\n");
                }
                FreeLibrary(hDLL);
            }
        }
    }
    Application::Destroy();

    atexit(&ReportLiveObjects);

    return retCode;
}