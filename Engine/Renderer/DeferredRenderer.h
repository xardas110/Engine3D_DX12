#pragma once
#include <Application.h>
#include <RenderTarget.h>
#include <Events.h>
#include <Raytracing.h>
#include <entt/entt.hpp>
#include <GBuffer/GBuffer.h>
#include <RootSignature.h>
#include <LightPass/LightPass.h>
#include <CompositionPass/CompositionPass.h>
#include <Debug/DebugTexture.h>

class Game;
class MeshTuple;
class CommandList;

namespace CameraParam
{
	enum
	{
		View,
		Proj,
		InvView,
		InvProj,
		Pos,
	};
}

class DeferredRenderer
{
	friend class Window;
	friend class World;
	friend class Editor;

	DeferredRenderer(int width, int height);
	~DeferredRenderer();

	void Render(class Window& window);

	void OnResize(ResizeEventArgs& e);

	int m_Width, m_Height; //shold be above the rest, cuz of initializing

	D3D12_RECT m_ScissorRect{ 0, 0, LONG_MAX, LONG_MAX };
	std::unique_ptr<Raytracing> m_Raytracer{nullptr};

	GBuffer m_GBuffer;
	LightPass m_LightPass;
	CompositionPass m_CompositionPass;
	DebugTexturePass m_DebugTexturePass;	

};

