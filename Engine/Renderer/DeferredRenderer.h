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
#include <NvidiaDenoiser.h>
#include <Skybox/Skybox.h>
#include <HDR/HDR.h>

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

	std::vector<MeshInstanceWrapper> GetMeshInstances(entt::registry& registry);

	int m_Width, m_Height; //should be above the rest, cuz of initializing

	D3D12_RECT m_ScissorRect{ 0, 0, LONG_MAX, LONG_MAX };
	std::unique_ptr<Raytracing> m_Raytracer{nullptr};

	GBuffer m_GBuffer;
	LightPass m_LightPass; //Init before denoiser
	CompositionPass m_CompositionPass;
	DebugTexturePass m_DebugTexturePass;	
	std::unique_ptr<NvidiaDenoiser> m_NvidiaDenoiser;

	CameraCB cameraCB;

	void SetRenderTexture(const Texture* texture)
	{
		renderTexture = texture;
	}

	const Texture* GetRenderTexture()
	{
		return renderTexture;
	}
	
	DenoiserTextures GetDenoiserTextures();

	const Texture* renderTexture{ nullptr };

	std::vector<Transform> prevTrans;

	std::unique_ptr<Skybox> m_Skybox;

};

