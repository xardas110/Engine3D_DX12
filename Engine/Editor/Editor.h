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
	friend class Window;
public:
	Editor(World* world); //for unique ptr
private:
	void ShowDockSpace(bool* p_open);
	
	void OnUpdate(UpdateEventArgs& e);

	void UpdateRenderSettings();

	void UpdateGameMenuBar();
	void UpdateRuntimeGame(UpdateEventArgs& e);

	void UpdateSceneGraph(entt::entity entity, const std::string& tag, RelationComponent& relation);

	void UpdateMaterialManager();

	void UpdateWorldHierarchy();
	void UpdateSelectedEntity();
	void UpdateMeshComponent(MeshComponent& mesh);

	void SelectEntity(entt::entity entity);

	//Default Game
	char gameDLL[20] = { "Sponzad" };
	
	//imgui settings
	struct ImUI
	{
		bool showDemoWindow = false;
		bool bShowGameLoader = true;

		bool bUseDocking = true;
		bool bDockFullScreen = true;
	} m_UI;

	//No ownership, safe ptr. Editor can't be alive without the ptr presisting.
	World* m_World{nullptr};

	entt::entity selectedEntity{ entt::null };
};
