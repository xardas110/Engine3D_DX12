#pragma once

#include <Events.h>

class Game;

class GameMode
{
	friend class World;
	
protected:
	GameMode(Game* game);

	virtual bool LoadContent() = 0;
	virtual void UnloadContent() = 0;

	virtual void OnBeginPlay();
	virtual void OnUpdate(UpdateEventArgs& e);
	virtual void OnGui(RenderEventArgs& e);
	Game* game;
};
