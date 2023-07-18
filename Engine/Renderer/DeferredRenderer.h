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
#include <Noise/BlueNoise.h>

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

	std::vector<MeshInstanceWrapper> GetMeshInstances(entt::registry& registry, std::vector<MeshInstanceWrapper>* pointLights = nullptr);

	void SetRenderTexture(const Texture* texture)
	{
		renderTexture = texture;
	}

	const Texture* GetRenderTexture()
	{
		return renderTexture;
	}
	
	DenoiserTextures GetDenoiserTextures();

	void DeferredRenderer::SetupDLSSResolution(int width, int height);

	void SetupScaledCameraJitter(const Camera* camera, XMFLOAT2& inoutJitterScaled);

	void SetupJitterMatrix(const XMFLOAT2& jitterScaled, XMMATRIX& inoutJitterMatrix);

	void SetupCameraConstantBuffer(const Camera* camera, const DirectX::XMFLOAT2& scaledCameraJitter);

	void SetupLightDataConstantBuffer(LightDataCB& lightDataCB, struct DirectionalLight& directionalLight, std::vector<MeshInstanceWrapper>& pointLights);

	void SetupRaytracingConstantBuffer(RaytracingDataCB& rtData, int listbox_item_debug, Window& window);

	void ClearRenderTargets(std::shared_ptr<CommandList>& commandList);

	void ExecuteAccelerationStructurePass(
		std::shared_ptr<CommandList>& commandList,
		std::vector<MeshInstanceWrapper>& meshInstances,
		Window& window);

	void ExecuteGBufferPass(
		std::shared_ptr<CommandList>& commandList, SRVHeapData& srvHeap,
		std::vector<MaterialColor>& materials, std::vector<MaterialInfoGPU>& globalMaterialInfo,
		std::vector<MeshInstanceWrapper>& meshInstances, ObjectCB& objectCB,
		const DirectX::XMMATRIX& jitterMat, std::vector<MeshInfo>& globalMeshInfo,
		AssetManager* assetManager, MeshManager::InstanceData& meshInstance);

	void ExcecuteLightPass(
		std::shared_ptr<CommandList>& commandList, SRVHeapData& srvHeap, 
		std::vector<MaterialColor>& materials, std::vector<MaterialInfoGPU>& globalMaterialInfo,
		std::vector<MeshInfo>& globalMeshInfo, struct DirectionalLight& directionalLight, 
		RaytracingDataCB& rtData, LightDataCB& lightDataCB);

	void ExecuteDenoisingPass(
		std::shared_ptr<CommandList>& commandList,
		const Camera* camera, Window& window);

	void ExecuteCompositionPass(
		std::shared_ptr<CommandList>& commandList, SRVHeapData& srvHeap,
		std::vector<MaterialColor>& materials, std::vector<MaterialInfoGPU>& globalMaterialInfo,
		std::vector<MeshInfo>& globalMeshInfo, RaytracingDataCB& rtData);

	void ExecuteHistogramExposurePass(std::shared_ptr<CommandList>& commandList);

	void ExecuteExposurePass(std::shared_ptr<CommandList>& commandList, const RenderEventArgs& e);

	void ExecuteDlssPass(std::shared_ptr<CommandList>& commandList, const Camera* camera);

	void ExecuteHDRPass(std::shared_ptr<CommandList>& commandList);

	void ExecuteDebugPass(std::shared_ptr<CommandList>& commandList, int& listbox_item_debug);

	void TransitionResourcesBackToRenderState(std::shared_ptr<CommandList>& commandList);

	void ExecuteCommandLists(std::shared_ptr<CommandQueue>& graphicsQueue, std::shared_ptr<CommandList>& commandList);


	/// \brief Caches the previous frame's data for use in motion vectors calculation and other temporal effects.
	///
	/// This function caches the previous frame's data which can be used in the next frame for various purposes. These uses include 
	/// the calculation of motion vectors, which require the positions from the current and the previous frame. The cached data 
	/// can also be used for other temporal effects like motion blur, temporal anti-aliasing, and more.
	///
	/// \param meshInstances The instances of meshes rendered in the current frame.
	/// \param cameraCB The camera's constant buffer data for the current frame.
	void CachePreviousFrameData(std::vector<MeshInstanceWrapper>& meshInstances, CameraCB& cameraCB);

	CameraCB m_CachedCameraCB{};

	const Texture* renderTexture{ nullptr };

	std::vector<Transform> m_LastFrameMeshTransforms{};

	D3D12_RECT m_ScissorRect{ 0, 0, LONG_MAX, LONG_MAX };

	std::unique_ptr<Skybox> m_Skybox;

	std::unique_ptr<GBuffer> m_GBuffer;
	std::unique_ptr<LightPass> m_LightPass; //Init before denoiser
	std::unique_ptr<CompositionPass> m_CompositionPass;
	
	std::unique_ptr<HDR> m_HDR;

	std::unique_ptr<DLSS> m_DLSS;
	std::unique_ptr<NvidiaDenoiser> m_NvidiaDenoiser;
	std::unique_ptr<Raytracing> m_Raytracer;

	std::unique_ptr<BlueNoise> m_BlueNoise;
	std::unique_ptr<DebugTexturePass> m_DebugTexturePass;

	int m_Width{}, m_Height{};
	int m_NativeWidth{}, m_NativeHeight{};

};

