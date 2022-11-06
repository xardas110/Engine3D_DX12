#pragma once

#include <Game.h>
#include <Window.h>
#include <DirectXMath.h>
#include <GameMode.h>
#include <Events.h>

class Sponza : public GameMode
{
public:

    Sponza(Game* game);
    virtual ~Sponza();

    /**
     *  Load content required for the demo.
     */
    virtual bool LoadContent() override;

    /**
     *  Unload demo specific content that was loaded in LoadContent.
     */
    virtual void UnloadContent() override;

    virtual void OnGui(RenderEventArgs& e) override;
protected:
    /**
     *  Update the game logic.
     */
    virtual void OnUpdate(UpdateEventArgs& e) override;
};
