#pragma once
#include <Texture.h>
#include <StaticDescriptorHeap.h>

class BlueNoise
{
	friend class DeferredRenderer;
private:

	void LoadTextures(std::shared_ptr<class CommandList>& commandList);

	Texture scrambling;
	Texture sobol;
	SRVHeapData heap = SRVHeapData(2);
};