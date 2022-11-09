#pragma once

#include <Events.h>
#include <entt/entt.hpp>
#include <Components.h>
#include <Window.h>
#include <GUI.h>

class World;

//This struct is only used for the editor layer
//Due to the viewports can be outside of the window bounds
struct GlobalInputs
{
	float mouseX{ 0.f };
	float mouseY{ 0.f };

	float previousMouseX{ 0.f };
	float previousMouseY{ 0.f };
};

class Editor
{
	friend class Application;

	void ShowDockSpace(bool* p_open);

	Editor(World* world);
	void Destroy(); //Destructor has to be public, Application will clean.

	void OnUpdate(UpdateEventArgs& e, std::shared_ptr<Window> window);
	void OnRender(RenderEventArgs& e, std::shared_ptr<Window> window);

	void RenderGui(UpdateEventArgs& e);
	void OnViewportRender(std::shared_ptr<Window> window);

	void OnResize(ResizeEventArgs& e, std::shared_ptr<Window>& window);

	//Global mouse pos, keys etc.
	void PollGlobalInputs();
	//viewport window inputs
	void PollWindowInputs(std::shared_ptr<Window> window);

	void UpdateGameMenuBar();
	void UpdateRuntimeGame(UpdateEventArgs& e);

	void UpdateSceneGraph(entt::entity entity, const std::string& tag, RelationComponent& relation);

	void UpdateWorldHierarchy();
	void UpdateSelectedEntity();

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

	struct ViewportData
	{
		float mousePosX{ 0.f };
		float mousePosY{ 0.f };

		int width{ 0 };
		int height{ 0 };

		bool bHovered{ false };
		bool bFocused{ false };
	};

	//Viewport data pr. window.
	std::map<std::wstring, ViewportData> m_ViewportDataMap;

	//Imgui system
	GUI m_Gui;

	//Poll global input will update this
	GlobalInputs m_GlobalInputs;

	//No ownership, safe ptr. Editor can't be alive without the ptr presisting.
	World* m_World{nullptr};

	entt::entity selectedEntity{ entt::null };
};