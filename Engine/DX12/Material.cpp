#include <EnginePCH.h>
#include "Material.h"
#include <Application.h>

MaterialInstanceID MaterialInstance::CreateMaterialInstance(const std::wstring& name, const MaterialInfo& textureIDs)
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;

	materialInstanceID = materialManager.CreateMaterialInstance(name, textureIDs);

	return materialInstanceID;
}

bool MaterialInstance::GetMaterialInstance(const std::wstring& name)
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;

	return materialManager.GetMaterialInstance(name, *this);
}

MaterialID MaterialInstance::CreateMaterial(const std::wstring& name, const Material& material)
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;
	return materialManager.CreateMaterial(name, material);
}

void MaterialInstance::SetMaterial(MaterialID materialId)
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;
	materialManager.SetMaterial(materialInstanceID, materialId);
}