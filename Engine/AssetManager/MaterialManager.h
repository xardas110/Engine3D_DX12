#pragma once
#include <RayTracingHlslCompat.h>
#include <Material.h>

class TextureData;

struct MaterialManager
{
	friend class AssetManager;
	friend class DeferredRenderer;

private:
	MaterialManager(const TextureManager& textureData);

	struct InstanceData
	{
		//1:1 relations. GPU info will be batched uploaded to gpu
		std::vector<MaterialInfo> cpuInfo;
		std::vector<MaterialInfo> gpuInfo;
		std::vector<UINT> refCounter;
		std::map<std::wstring, MaterialInstanceID> map;
	} instanceData;

	bool CreateMaterialInstance(const std::wstring& name, const MaterialInfo& textureIDs);
	bool GetMaterialInstance(const std::wstring& name, MaterialInstance& outMaterialInstance);

	//RefCounting access. 
	const TextureManager& m_TextureManager;
};