#pragma once

#include <Events.h>
#include <entt/entt.hpp>
#include <Components.h>
#include <Window.h>
#include <GUI.h>

class World;

class Editor
{
	friend class Application;

	void ShowDockSpace(bool* p_open);

	Editor(World* world);
	void Destroy(); //Destructor needs to be public, destruction is by application.

	void RenderGui(UpdateEventArgs& e);
	void OnViewportRender(const Texture& sceneTexture);

	void OnUpdate(UpdateEventArgs& e, std::shared_ptr<Window> window);
	void OnRender(RenderEventArgs& e, std::shared_ptr<Window> window);

	void UpdateGameMenuBar();
	void UpdateRuntimeGame(UpdateEventArgs& e);

	void UpdateSceneGraph(entt::entity entity, const std::string& tag, RelationComponent& relation);

	void UpdateWorldHierarchy();
	void UpdateSelectedEntity();

	void SelectEntity(entt::entity entity);

	//Default Game
	char gameDLL[20] = { "Sponzad" };
	
	//Menu items
	bool showDemoWindow = false;
	bool bShowGameLoader = true;

	bool bUseDocking = true;
	bool bDockFullScreen = true;

	//Imgui system
	GUI m_Gui;

	//No ownership, safe ptr. Editor can't be alive without the ptr presisting.
	World* m_World{nullptr};

	entt::entity selectedEntity{ entt::null };
};