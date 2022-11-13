#include <EnginePCH.h>
#include "Material.h"
#include <Application.h>

MaterialInstanceID MaterialInstance::CreateMaterialInstance(const std::wstring& name, const MaterialInfo& textureIDs)
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;

	materialID = materialManager.CreateMaterialInstance(name, textureIDs);

	return materialID;
}

bool MaterialInstance::GetMaterialInstance(const std::wstring& name)
{
	auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;

	return materialManager.GetMaterialInstance(name, *this);
}
