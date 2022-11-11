#pragma once

#include <Events.h>
#include <entt/entt.hpp>
#include <Components.h>
#include <Window.h>
#include <GUI.h>

#ifdef DEBUG_EDITOR
class World;

class Editor
{
	friend class Application;

	void ShowDockSpace(bool* p_open);

	Editor(World* world);
	void Destroy(); //Destructor has to be public, Application will clean.

	//Invoked per frame
	void OnUpdate(UpdateEventArgs& e);
	void OnRender(RenderEventArgs& e);
	//--

	void RenderGui(UpdateEventArgs& e);
	void OnViewportRender(std::shared_ptr<Window> window);

	void OnResize(ResizeEventArgs& e, std::shared_ptr<Window>& window);

	//Must write my own input polling since imgui viewport is used
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

		bool bShift{ false };
		bool bControl{ false };
		bool bAlt{ false };

		float mouseGlobalPosX{ 0.f };
		float mouseGlobalPosY{ 0.f };

		//keymap for global key input
		std::map<KeyCode::Key, bool> keys;
	};

	//Viewport data pr. window.
	std::map<std::wstring, ViewportData> m_ViewportDataMap;
	//Last frame viewport data pr. window.
	std::map<std::wstring, ViewportData> m_PreviousViewportDataMap;

	HighResolutionClock m_UpdateClock;
	HighResolutionClock m_RenderClock;

	//Imgui system
	GUI m_Gui;

	//No ownership, safe ptr. Editor can't be alive without the ptr presisting.
	World* m_World{nullptr};

	entt::entity selectedEntity{ entt::null };
};
#endif // DEBUG_EDITOR

