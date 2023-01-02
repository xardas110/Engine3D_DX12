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
#include <DLSS/DLSS.h>

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

	void LoadContent();

	void Render(class Window& window, const RenderEventArgs& e);

	void OnResize(ResizeEventArgs& e);

	//Custom Subscription event sent by DLSS struct
	void OnDlssChanged(const bool& bDlssOn, const NVSDK_NGX_PerfQuality_Value& qualityMode);

	void Shutdown();

	std::vector<MeshInstanceWrapper> GetMeshInstances(entt::registry& registry);

	void SetRenderTexture(const Texture* texture)
	{
		renderTexture = texture;
	}

	const Texture* GetRenderTexture()
	{
		return renderTexture;
	}
	
	DenoiserTextures GetDenoiserTextures();

	int m_Width, m_Height;
	int m_NativeWidth, m_NativeHeight;

	CameraCB cameraCB;

	const Texture* renderTexture{ nullptr };

	std::vector<Transform> prevTrans;

	D3D12_RECT m_ScissorRect{ 0, 0, LONG_MAX, LONG_MAX };

	std::unique_ptr<Skybox> m_Skybox;

	std::unique_ptr<GBuffer> m_GBuffer;
	std::unique_ptr<LightPass> m_LightPass; //Init before denoiser
	std::unique_ptr<CompositionPass> m_CompositionPass;
	
	std::unique_ptr<HDR> m_HDR;

	std::unique_ptr<DLSS> m_DLSS;
	std::unique_ptr<NvidiaDenoiser> m_NvidiaDenoiser;
	std::unique_ptr<Raytracing> m_Raytracer;

	std::unique_ptr<DebugTexturePass> m_DebugTexturePass;

	Texture scrambling;
	Texture sobol;
	SRVHeapData heap = SRVHeapData(2);
};

