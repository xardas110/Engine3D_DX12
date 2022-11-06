#pragma once

#include <Events.h>
#include <entt/entt.hpp>

class World;

class Editor
{
	friend class World;

	Editor(World* world);

	void OnGui(RenderEventArgs& e);
	void UpdateGameMenuBar(RenderEventArgs& e);
	void UpdateRuntimeGame(RenderEventArgs& e);
	void UpdateWorldHierarchy(RenderEventArgs& e);

	void SelectEntity(entt::entity entity);

	//Default Game
	char gameDLL[20] = { "Sponza" };
	
	//Menu items
	bool showDemoWindow = false;
	bool bShowGameLoader = true;

	//No ownership, safe ptr. Editor can't be alive without the ptr presisting.
	World* m_World{nullptr};

	entt::entity selectedEntity{ entt::null };
};