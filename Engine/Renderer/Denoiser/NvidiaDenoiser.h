#pragma once
#include <Texture.h>
#include <LightPass/LightPass.h>
#include <TypesCompat.h>
#include <CommandList.h>

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

	NvidiaDenoiser();
	~NvidiaDenoiser();

	void Init(int width, int height, LightPass& lightPass);

	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_Adapter;	

	void RenderFrame(CommandList& commandList, const CameraCB& cam, DenoiserTextures& texs, int currentBackbufferIndex, int width, int height);
	
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;

};