#pragma once

#include <Events.h>
#include <entt/entt.hpp>
#include <Components.h>

class World;

class Editor
{
	friend class World;

	Editor(World* world);

	void OnGui(RenderEventArgs& e);
	void UpdateGameMenuBar(RenderEventArgs& e);
	void UpdateRuntimeGame(RenderEventArgs& e);

	void UpdateSceneGraph(entt::entity entity, const std::string& tag, RelationComponent& relation);

	void UpdateWorldHierarchy(RenderEventArgs& e);
	void UpdateSelectedEntity(RenderEventArgs& e);

	void SelectEntity(entt::entity entity);

	//Default Game
	char gameDLL[20] = { "Sponzad" };
	
	//Menu items
	bool showDemoWindow = false;
	bool bShowGameLoader = true;

	//No ownership, safe ptr. Editor can't be alive without the ptr presisting.
	World* m_World{nullptr};

	entt::entity selectedEntity{ entt::null };
};