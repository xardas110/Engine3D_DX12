#pragma once

#include <Events.h>

class World;

class Editor
{
	friend class World;

	Editor(World* world);
	
	void OnGui(RenderEventArgs& e);
	void UpdateGameMenuBar(RenderEventArgs& e);
	void UpdateRuntimeGame(RenderEventArgs& e);

	//Default Game
	char gameDLL[20] = { "Sponza" };
	
	//Menu items
	bool showDemoWindow = false;
	bool bShowGameLoader = true;

	//No ownership, safe ptr
	World* m_World;
};