#include <EnginePCH.h>
#include <MaterialManager.h>
#include <TextureManager.h>

bool IsMaterialValid(UINT id)
{
	return id != UINT_MAX;
}

MaterialManager::MaterialManager(const TextureManager& textureManager)
	:m_TextureManager(textureManager)
{
}

MaterialInstanceID MaterialManager::CreateMaterialInstance(const std::wstring& name, const MaterialInfo& textureIDs)
{
	assert(instanceData.map.find(name) == instanceData.map.end() && "Material instance exists!");
	if (instanceData.map.find(name) != instanceData.map.end()) return UINT_MAX;

	const auto currentIndex = instanceData.cpuInfo.size();
	instanceData.cpuInfo.emplace_back(textureIDs);
	instanceData.gpuInfo.emplace_back(MaterialInfo());
	instanceData.refCounter.emplace_back(UINT(1U));

	const auto& textureData = m_TextureManager.textureData;

	if (IsMaterialValid(textureIDs.albedo))
	{
		instanceData.gpuInfo[currentIndex].albedo = textureData.textures[textureIDs.albedo].heapID;

		m_TextureManager.IncrementRef(textureIDs.albedo);

	}
	if (IsMaterialValid(textureIDs.normal))
	{
		instanceData.gpuInfo[currentIndex].normal = textureData.textures[textureIDs.normal].heapID;

		m_TextureManager.IncrementRef(textureIDs.normal);
	}

	instanceData.map[name] = currentIndex;

	return currentIndex;
}

bool MaterialManager::GetMaterialInstance(const std::wstring& name, MaterialInstance& outMaterialInstance)
{
	assert(instanceData.map.find(name) != instanceData.map.end() && "Unable to find material instance");

	if (instanceData.map.find(name) == instanceData.map.end()) return false;

	outMaterialInstance.materialID = instanceData.map[name];

	return true;
}
