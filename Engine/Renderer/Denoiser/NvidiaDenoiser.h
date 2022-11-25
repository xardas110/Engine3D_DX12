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
		const Texture& outIndirectDiffuse, const Texture& outIndirectSpecular)
		:	inMV(inMV), 
			inViewZ(inViewZ),
			inNormalRough(inNormalRough), 
			inIndirectDiffuse(inIndirectDiffuse),
			inIndirectSpecular(inIndirectSpecular), 
			outIndirectDiffuse(outIndirectDiffuse),
			outIndirectSpecular(outIndirectSpecular) {};

	const Texture& inMV;
	const Texture& inViewZ;
	const Texture& inNormalRough;
	const Texture& inIndirectDiffuse;
	const Texture& inIndirectSpecular;
	const Texture& outIndirectDiffuse;
	const Texture& outIndirectSpecular;
};

class NvidiaDenoiser
{
	friend class DeferredRenderer;
	friend struct std::default_delete<NvidiaDenoiser>;

	NvidiaDenoiser(int width, int height);
	~NvidiaDenoiser();

	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_Adapter;	

	void RenderFrame(CommandList& commandList, const CameraCB& cam, DenoiserTextures& texs, int currentBackbufferIndex, int width, int height);
	
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;

	NrdIntegration NRD = NrdIntegration(3);

	struct NriInterface
		: public nri::CoreInterface
		, public nri::HelperInterface
		, public nri::WrapperD3D12Interface
	{};

	nri::Device* m_NRIDevice;

	NriInterface NRI;

	nrd::CommonSettings commonSettings = {};

	nri::CommandBuffer* nriCommandBuffer = nullptr;
};