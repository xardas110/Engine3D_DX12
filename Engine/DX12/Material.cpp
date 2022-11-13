#include <EnginePCH.h>
#include "Material.h"
#include <Application.h>

bool MaterialInstance::CreateMaterialInstance(const std::wstring& name, const MaterialInfo& textureIDs)
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;

	return materialManager.CreateMaterialInstance(name, textureIDs);
}

bool MaterialInstance::GetMaterialInstance(const std::wstring& name)
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;

	return GetMaterialInstance(name);
}
