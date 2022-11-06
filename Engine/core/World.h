#pragma once
#include <GameMode.h>
#include <Game.h>

class Editor;

typedef GameMode* (CALLBACK* GameEntry)(Game*);

/**
*  GameWorld class, presitent trought different games.
*  Runs all the systems(Physics, Audio, Rendering etc.)
*/
class World : public Game
{
    friend class Editor;
public:
    using super = Game;

    World(const std::wstring& name, int width, int height, bool vSync = false);
        
    virtual ~World() {}

    /**
     *  Load gamemode content from dll.
     */
    void LoadGameMode();

    /**
     *  Unload gamemode content from dll.
     */
    void UnLoadGameMode();

    /**
     *  Load content required.
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
    virtual void OnUpdate(UpdateEventArgs& e) override;

    /**
     *  Update the game logic.
     */
    virtual void OnRender(RenderEventArgs& e) override;

private:
    /**
     *  Load compatible runtime game dll.
     */
    bool LoadRuntimeGame(const std::string& name);

    /**
     *  Unload game dll.
     */
    void UnloadRuntimeGame();

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

    std::unique_ptr<GameMode> m_GameMode{ nullptr };

#ifdef DEBUG_EDITOR
    std::unique_ptr<Editor> m_Editor{ nullptr};
#endif // DEBUG_EDITOR
};