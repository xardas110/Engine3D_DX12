#pragma once
#include <Application.h>
#include <RenderTarget.h>
#include <Events.h>
#include <Raytracing.h>
#include <entt/entt.hpp>

class Game;
class MeshTuple;
class CommandList;

class DeferredRenderer
{
	friend class Window;
	friend class World;
	friend class Editor;

	DeferredRenderer(int width, int height);
	~DeferredRenderer();

	void CreateGBuffer();
	void Render(class Window& window);

	void OnResize(ResizeEventArgs& e);

	int m_Width, m_Height;
	RenderTarget m_GBufferRenderTarget;
	D3D12_RECT m_ScissorRect{ 0, 0, LONG_MAX, LONG_MAX };

	std::unique_ptr<Raytracing> m_Raytracer{nullptr};
	Texture m_DXTexture;
};

