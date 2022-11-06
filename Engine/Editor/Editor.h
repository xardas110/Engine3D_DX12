#pragma once

#include <Events.h>

class World;

class Editor
{
	friend class World;

	Editor(World* world);

	void OnGui(RenderEventArgs& e);

	//No ownership, safe ptr
	World* m_World;
};