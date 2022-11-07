#include <EnginePCH.h>
#include <World.h>
#include <Editor.h>
#include <Application.h>

World::World(const std::wstring& name, int width, int height, bool vSync)
    :Game(name, width, height, vSync)
{
#ifdef DEBUG_EDITOR
    m_Editor = std::unique_ptr<Editor>(new Editor(this));
#endif
}

void World::LoadGameMode()
{
	if (m_GameMode)
    { 
        ClearGameWorld();
		m_GameMode->LoadContent();
    }
}

void World::UnLoadGameMode()
{
	if (m_GameMode)
		m_GameMode->UnloadContent();
}

void World::OnUpdate(UpdateEventArgs& e)
{
	super::OnUpdate(e);

    if (m_GameMode)
        m_GameMode->OnUpdate(e);
}

void World::OnRender(RenderEventArgs& e)
{
	super::OnRender(e);
#ifdef DEBUG_EDITOR
    m_Editor->OnGui(e);
    if (m_GameMode) m_GameMode->OnGui(e);
#endif
}

bool World::LoadRuntimeGame(const std::string& name)
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

    m_GameMode = std::unique_ptr<GameMode>(gameEntry(this, &Application::Get()));
    LoadGameMode();
}

void World::UnloadRuntimeGame()
{
    UnLoadGameMode();
    FreeLibrary(runTimeGame.hDLL);
    runTimeGame.Reset();
    m_GameMode = nullptr;
}
