#pragma once
#include <TypesCompat.h>
#include <Material.h>

class TextureData;

struct MaterialManager
{
	friend class AssetManager;
	friend class DeferredRenderer;

	//Materialinfo needs to be initalized with 0
	MaterialInstanceID CreateMaterialInstance(const std::wstring& name, const MaterialInfo& textureIDs);
	bool GetMaterialInstance(const std::wstring& name, MaterialInstance& outMaterialInstance);

private:
	MaterialManager(const TextureManager& textureData);

	struct MaterialData
	{
		//std::vector<Material> materials;
	};

	struct InstanceData
	{
		//1:1 relations. GPU info will be batched uploaded to gpu
		std::vector<MaterialInfo> cpuInfo;
		std::vector<MaterialInfo> gpuInfo;	

		std::vector<UINT> refCounter;
		std::map<std::wstring, MaterialInstanceID> map;
		
	} instanceData;

	const TextureManager& m_TextureManager;
};