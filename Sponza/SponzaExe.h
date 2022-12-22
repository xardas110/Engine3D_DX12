#pragma once

#include <Game.h>
#include <Window.h>
#include <DirectXMath.h>
#include <GameMode.h>
#include <Events.h>

class SponzaExe : public Game
{
public:

    SponzaExe(const std::wstring& name, int width, int height, bool vSync);
    virtual ~SponzaExe();

    /**
     *  Load content required for the demo.
     */
    virtual bool LoadContent() override;

    /**
     *  Unload demo specific content that was loaded in LoadContent.
     */
    virtual void UnloadContent() override;

protected:
    /**
     *  Update the game logic.
     */
    virtual void OnUpdate(UpdateEventArgs& e) override;

    float moveAmount = 5.f;
    float currentMove = 0.f;
    int sign = 1;

    std::vector<Transform> initalPositions;
    std::vector<std::uint32_t> lights;
};

