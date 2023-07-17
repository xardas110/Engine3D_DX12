#pragma once
#include <TypesCompat.h>
#include <Material.h>

class TextureData;

struct MaterialManager
{
	friend class AssetManager;
	friend class DeferredRenderer;
	friend class Editor;
	friend class MaterialInstance;

	MaterialInstanceID CreateMaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs);
	bool GetMaterialInstance(const std::wstring& name, MaterialInstance& outMaterialInstance);

	MaterialID CreateMaterial(const std::wstring& name, const Material& material);

	MaterialID GetMaterial(const std::wstring& name);

	void SetMaterial(MaterialID materialId, MaterialInstanceID matInstanceID);

	TextureInstance GetTextureInstance(MaterialType::Type type, MaterialInstanceID matInstanceId);

	const std::wstring& GetMaterialInstanceName(MaterialInstanceID matInstanceId) const;

	const std::wstring& GetMaterialName(MaterialInstanceID matInstanceId) const;

	Material& GetUserDefinedMaterial(MaterialInstanceID matInstanceId);
private:
	MaterialManager(const TextureManager& textureData);

	struct MaterialData
	{
		std::vector<Material> materials;
		std::map<std::wstring, MaterialID> map;
	} materialData;

	struct InstanceData
	{
		//1:1 relations. GPU info will be batched to gpu
		std::vector<MaterialInfoCPU> cpuInfo;
		std::vector<MaterialInfoGPU> gpuInfo;	

		std::vector<UINT> refCounter;
		std::map<std::wstring, MaterialInstanceID> map;
		
	} instanceData;

	const TextureManager& m_TextureManager;
};