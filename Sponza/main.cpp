#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <Shlwapi.h>

#include <Application.h>
#include <World.h>
#include <GameMode.h>
#include <dxgidebug.h>
#include <filesystem>
#include <iostream>
#include <Window.h>
#include <Editor.h>
#include <SponzaExe.h>

void ReportLiveObjects()
{
    IDXGIDebug1* dxgiDebug;
    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    dxgiDebug->Release();
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

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
        auto world = std::shared_ptr<SponzaExe>(new SponzaExe(L"Sponza", 1280, 720, false));
        Application::Get().Run(world);
    }
    Application::Destroy();

    atexit(&ReportLiveObjects);

    return retCode;
}

