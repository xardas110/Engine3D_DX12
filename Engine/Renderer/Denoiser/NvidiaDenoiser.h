#pragma once
#include <Texture.h>
#include <LightPass/LightPass.h>
#include <TypesCompat.h>

class NvidiaDenoiser
{
	friend class DeferredRenderer;

	NvidiaDenoiser();
	~NvidiaDenoiser();

	void Init(int width, int height, LightPass& lightPass);

	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_Adapter;	
	std::shared_ptr<CommandList> m_CommandList;

	void RenderFrame(const CameraCB& cam, LightPass& lightPass, int currentBackbufferIndex);
	
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;

	int width = 0;
	int height = 0;
};