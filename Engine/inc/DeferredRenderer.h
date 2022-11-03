#pragma once
#include <Application.h>
#include <RenderTarget.h>
#include <Events.h>

class Game;

class DeferredRenderer
{
	friend class Window;

	DeferredRenderer(int width, int height);
	~DeferredRenderer();

	void CreateGBuffer();
	void Render(std::shared_ptr<Game>& game);

	void OnResize(ResizeEventArgs& e);

	RenderTarget m_GBufferRenderTarget;
	int m_Width, m_Height;
};

