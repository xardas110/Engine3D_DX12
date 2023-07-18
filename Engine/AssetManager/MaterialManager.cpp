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

std::optional<MaterialID> MaterialManager::CreateMaterialInstance(const std::wstring& name, const MaterialInfoCPU& textureIDs, const MaterialColor& materialColor)
{
	if (materialRegistry.map.find(name) != materialRegistry.map.end())
	{
		// Optional without a value indicates an error
		return std::nullopt;
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

bool MaterialManager::GetMaterialInstance(const std::wstring& name, MaterialInstance& outMaterialInstance)
{
	if (materialRegistry.map.find(name) == materialRegistry.map.end()) return false;
	outMaterialInstance.materialID = materialRegistry.map[name];
	return true;
}

MaterialID MaterialManager::GetMaterial(const std::wstring& name)
{
	if (materialRegistry.map.find(name) != materialRegistry.map.end()) return UINT_MAX;
	return materialRegistry.map[name];
}

void MaterialManager::SetFlags(MaterialID materialId, const UINT flags)
{
	materialRegistry.gpuInfo[materialId].flags = flags;
	materialRegistry.cpuInfo[materialId].flags = flags;
}

void MaterialManager::AddFlags(MaterialID materialID, const UINT flags)
{
	materialRegistry.gpuInfo[materialID].flags |= flags;
	materialRegistry.cpuInfo[materialID].flags |= flags;
}

TextureInstance MaterialManager::GetTextureInstance(MaterialType::Type type, MaterialID matInstanceId)
{
	auto& matInfo = materialRegistry.cpuInfo[matInstanceId];
	return matInfo.GetTexture(type);
}

const std::wstring& MaterialManager::GetMaterialInstanceName(MaterialID matInstanceId) const
{
	if (matInstanceId == UINT_MAX) return g_NoMaterial;

	for (const auto& [name, id] : materialRegistry.map)
	{
		if (id == matInstanceId) return name;
	}

	return g_NoMaterial;
}

const MaterialColor& MaterialManager::GetMaterialColor(const MaterialID materialID)
{
	if (materialID == UINT_MAX)
	{
		return g_NoUserMaterial;
	}

	return materialRegistry.materialColors[materialID];
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