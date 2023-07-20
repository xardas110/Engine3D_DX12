#include <EnginePCH.h>
#include <MaterialManager.h>
#include <TextureManager.h>
#include <Texture.h>

std::wstring g_NoMaterial = L"No Material";
MaterialColor g_NoUserMaterial; //Already initialized with values

bool IsMaterialValid(UINT id)
{
	return id != UINT_MAX;
}

void PopulateGPUInfo(MaterialInfoGPU& gpuInfo, const TextureInstance& instance, UINT& gpuField)
{
	if (instance.IsValid())
	{
		gpuField = instance.GetTextureGPUHandle().value();
	}
}

const MaterialInstance& MaterialManager::LoadMaterial(const std::wstring& name, const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor)
{
	if (materialRegistry.map.find(name) != materialRegistry.map.end())
	{
		return materialRegistry.map[name];
	}

	const auto currentIndex = materialRegistry.cpuInfo.size();
	materialRegistry.cpuInfo.emplace_back(textureIDs);
	materialRegistry.gpuInfo.emplace_back(MaterialInfoGPU());
	materialRegistry.materialColors.emplace_back(materialColor);

	assert(materialRegistry.cpuInfo.size() == materialRegistry.gpuInfo.size());
	assert(materialRegistry.gpuInfo.size() == materialRegistry.materialColors.size());

	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.GetTexture(MaterialType::albedo), materialRegistry.gpuInfo[currentIndex].albedo);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.GetTexture(MaterialType::normal), materialRegistry.gpuInfo[currentIndex].normal);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.GetTexture(MaterialType::ao), materialRegistry.gpuInfo[currentIndex].ao);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.GetTexture(MaterialType::emissive), materialRegistry.gpuInfo[currentIndex].emissive);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.GetTexture(MaterialType::roughness), materialRegistry.gpuInfo[currentIndex].roughness);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.GetTexture(MaterialType::specular), materialRegistry.gpuInfo[currentIndex].specular);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.GetTexture(MaterialType::metallic), materialRegistry.gpuInfo[currentIndex].metallic);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.GetTexture(MaterialType::lightmap), materialRegistry.gpuInfo[currentIndex].lightmap);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.GetTexture(MaterialType::opacity), materialRegistry.gpuInfo[currentIndex].opacity);
	PopulateGPUInfo(materialRegistry.gpuInfo[currentIndex], textureIDs.GetTexture(MaterialType::height), materialRegistry.gpuInfo[currentIndex].height);

	materialRegistry.map[name] = currentIndex;

	return currentIndex;
}

std::optional<MaterialInstance> MaterialManager::GetMaterialInstance(const std::wstring& name)
{
	if (materialRegistry.map.find(name) == materialRegistry.map.end()) 
		return std::nullopt;

	return materialRegistry.map[name];
}

std::optional<MaterialInstance> MaterialManager::GetMaterial(const std::wstring& name)
{
	if (materialRegistry.map.find(name) != materialRegistry.map.end()) 
		return std::nullopt;

	return materialRegistry.map[name];
}

void MaterialManager::SetFlags(const MaterialInstance& materialInstance, const UINT flags)
{
	materialRegistry.gpuInfo[materialInstance.materialID].flags = flags;
	materialRegistry.cpuInfo[materialInstance.materialID].flags = flags;
}

void MaterialManager::AddFlags(const MaterialInstance& materialInstance, const UINT flags)
{
	materialRegistry.gpuInfo[materialInstance.materialID].flags |= flags;
	materialRegistry.cpuInfo[materialInstance.materialID].flags |= flags;
}

TextureInstance MaterialManager::GetTextureInstance(const MaterialInstance& materialInstance, MaterialType::Type type)
{
	auto& matInfo = materialRegistry.cpuInfo[materialInstance.materialID];
	return matInfo.GetTexture(type);
}

const std::wstring& MaterialManager::GetMaterialName(const MaterialInstance& materialInstance) const
{
	if (materialInstance.materialID == MATERIAL_INVALID)
		return g_NoMaterial;

	for (const auto& [name, id] : materialRegistry.map)
	{
		if (id == materialInstance.materialID)
			return name;
	}

	return g_NoMaterial;
}

const MaterialColor& MaterialManager::GetMaterialColor(const MaterialInstance& materialInstance)
{
	if (materialInstance.materialID == MATERIAL_INVALID)
	{
		return g_NoUserMaterial;
	}

	return materialRegistry.materialColors[materialInstance.materialID];
}

const std::vector<MaterialInfoCPU>& MaterialManager::GetMaterialCPUInfoData() const
{
	return materialRegistry.cpuInfo;
}

const std::vector<MaterialInfoGPU>& MaterialManager::GetMaterialGPUInfoData() const
{
	return materialRegistry.gpuInfo;
}

const std::vector<MaterialColor>& MaterialManager::GetMaterialColorData() const
{
	return materialRegistry.materialColors;
}