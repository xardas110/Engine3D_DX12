#pragma once
#include <Texture.h>

class NvidiaDenoiser
{
	friend class DeferredRenderer;

	NvidiaDenoiser();
	~NvidiaDenoiser();

	void Init(int width, int height, const Texture& diffuseSpecular);

	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_Adapter;	
};