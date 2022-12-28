#pragma once
#include <Texture.h>
#include <LightPass/LightPass.h>
#include <TypesCompat.h>
#include <CommandList.h>

#include <NRI.h>

#include <Extensions/NRIWrapperD3D12.h>
#include <Extensions/NRIHelper.h>

#include <NRD.h>
#include <NRDIntegration.h>

#include <NRDSettings.h>

struct DenoiserTextures
{
	DenoiserTextures(const Texture& inMV, const Texture& inViewZ, const Texture& inNormalRough,
		const Texture& inIndirectDiffuse, const Texture& inIndirectSpecular,
		const Texture& outIndirectDiffuse, const Texture& outIndirectSpecular,
		const Texture& inShadowData, const Texture& outShadow)
		:	inMV(inMV), 
			inViewZ(inViewZ),
			inNormalRough(inNormalRough), 
			inIndirectDiffuse(inIndirectDiffuse),
			inIndirectSpecular(inIndirectSpecular), 
			outIndirectDiffuse(outIndirectDiffuse),
			outIndirectSpecular(outIndirectSpecular),
			inShadowData(inShadowData),
			outShadow(outShadow)
			
	{};

	const Texture& inMV;
	const Texture& inViewZ;
	const Texture& inNormalRough;
	const Texture& inIndirectDiffuse;
	const Texture& inIndirectSpecular;
	const Texture& outIndirectDiffuse;
	const Texture& outIndirectSpecular;

	const Texture& inShadowData;
	const Texture& outShadow;
};

namespace nriTypes
{
	enum Type
	{
		inMV,
		inNormalRoughness,
		inViewZ,
		inIndirectDiffuse,
		inIndirectSpecular,
		outIndirectDiffuse,
		outIndirectSpecular,
		inShadowData,
		outShadow,
		size
	};
}

struct NRITextures
{
	nri::Texture* texture;
	nri::TextureTransitionBarrierDesc entryDescs = {};
	nri::Format entryFormat = {};
};

struct DenoiserSettings
{
	DenoiserSettings()
	{
		settings.blurRadius = 60.f;
		commonSettings.denoisingRange = 50002.f;
		settings.hitDistanceReconstructionMode = nrd::HitDistanceReconstructionMode::AREA_3X3;
		settings.antilagHitDistanceSettings.enable = true;
		//settings.antilagIntensitySettings.enable = true;	
		//settings.antilagIntensitySettings.sensitivityToDarkness = 0.1f;
		//settings.hitDistanceParameters.A = 1.f;
		//settings.hitDistanceParameters.B = 1.f;
		//settings.maxAccumulatedFrameNum = 30;
		//settings.maxFastAccumulatedFrameNum = 30;
		//settings.historyFixFrameNum = 15;
		settings.diffusePrepassBlurRadius = 60.f;
		settings.specularPrepassBlurRadius = 60.f;
	}

	nrd::ReblurSettings settings = {};
	nrd::CommonSettings commonSettings = {};
};

class NvidiaDenoiser
{
	friend class DeferredRenderer;
	friend struct std::default_delete<NvidiaDenoiser>;

	NvidiaDenoiser(int width, int height, DenoiserTextures& texs);
	~NvidiaDenoiser();

	void RenderFrame(CommandList& commandList, const CameraCB& cam, int currentBackbufferIndex, int width, int height);
	void OnGUI();

	NrdIntegration NRD = NrdIntegration(3);
	NrdIntegration NRD_Sigma = NrdIntegration(3);

	struct NriInterface
		: public nri::CoreInterface
		, public nri::HelperInterface
		, public nri::WrapperD3D12Interface
	{};

	nri::Device* m_NRIDevice;

	NriInterface NRI;

	nri::CommandBuffer* nriCommandBuffer = nullptr;

	NRITextures nriTextures[nriTypes::size];

	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_Adapter;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;

	const int width, height;
	DenoiserSettings denoiserSettings;

	Texture scrambling;
	Texture sobol;
};