#include <EnginePCH.h>
#include "GameMode.h"
#include <Game.h>

GameMode::GameMode(Game* game)
	:game(game)
{}

void GameMode::OnBeginPlay()
{}

void GameMode::OnUpdate(UpdateEventArgs& e)
{}

void GameMode::OnGui(RenderEventArgs& e)
{}
