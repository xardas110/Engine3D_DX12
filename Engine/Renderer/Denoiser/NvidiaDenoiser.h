#pragma once
#include <Texture.h>
#include <LightPass/LightPass.h>
#include <TypesCompat.h>
#include <CommandList.h>

class NvidiaDenoiser
{
	friend class DeferredRenderer;

	NvidiaDenoiser();
	~NvidiaDenoiser();

	void Init(int width, int height, LightPass& lightPass);

	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_Adapter;	

	void RenderFrame(CommandList& commandList, const CameraCB& cam, LightPass& lightPass, int currentBackbufferIndex, int width, int height);
	
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
	Microsoft::WRL::ComPtr<ID3D12CommandList> m_CommandList;
};