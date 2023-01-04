#include <EnginePCH.h>
#include <LightManager.h>
#include <Application.h>

void LightManger::LoadContent()
{
    auto& meshManager = Application::Get().GetAssetManager()->m_MeshManager;

    meshManager.CreateSphere();

    Material materialEmissive;
    materialEmissive.diffuse = { 1.f, 1.f, 1.f, 1.f };
    materialEmissive.emissive = { 0.f, 0.f, 0.f };
    materialEmissive.roughness = 0.f;
    materialEmissive.metallic = 1.f;

    auto materialID = MaterialInstance::CreateMaterial(L"DefaultEmissiveRed", materialEmissive);

    MaterialInstance material;
    MaterialInfo matInfo;
    material.CreateMaterialInstance(L"DefaultMaterialInstance", matInfo);
    material.SetMaterial(materialID);
    material.SetFlags(INSTANCE_LIGHT);

    pointLightMesh = MeshInstance(L"DefaultSphere");

    pointLightMesh.SetMaterialInstance(materialID);

}
