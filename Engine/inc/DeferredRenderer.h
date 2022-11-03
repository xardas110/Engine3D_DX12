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

	int m_Width, m_Height;
	RenderTarget m_GBufferRenderTarget;
	D3D12_RECT m_ScissorRect{ 0, 0, LONG_MAX, LONG_MAX };
};

