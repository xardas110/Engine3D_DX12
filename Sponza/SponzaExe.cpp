#include "SponzaExe.h"
#include <iostream>
#include <Components.h>
#include <entity.h>

SponzaExe::SponzaExe(const std::wstring& name, int width, int height, bool vSync)
	:Game(name, width, height, vSync)
{
    srand(time(nullptr));
}

SponzaExe::~SponzaExe()
{
}

bool SponzaExe::LoadContent()
{
	std::cout << "Sponza exe running " << std::endl;


    auto& meshManager = Application::Get().GetAssetManager()->m_MeshManager;
    auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;

    meshManager.CreateCube();
    meshManager.CreateSphere();

    std::cout << "game init" << std::endl;

    Material materialRed;
    materialRed.color = { 1.f, 0.f, 0.f, 1.f };

    auto materialID = MaterialInstance::CreateMaterial(L"RED", materialRed);

    TextureInstance spaceAlbedo(L"Assets/Textures/SpaceBall/space-cruiser-panels2_albedo.png");
    TextureInstance spaceNormal(L"Assets/Textures/SpaceBall/space-cruiser-panels2_normal-dx.png");
    TextureInstance spaceAO(L"Assets/Textures/SpaceBall/space-cruiser-panels2_ao.png");
    TextureInstance spaceRoughness(L"Assets/Textures/SpaceBall/space-cruiser-panels2_roughness.png");
    TextureInstance spaceMetallic(L"Assets/Textures/SpaceBall/space-cruiser-panels2_metallic.png");
    TextureInstance spaceHeight(L"Assets/Textures/SpaceBall/space-cruiser-panels2_height.png");

    MaterialInfo matInfo;
    matInfo.albedo = spaceAlbedo.GetTextureID();
    matInfo.normal = spaceNormal.GetTextureID();
    matInfo.metallic = spaceMetallic.GetTextureID();
    matInfo.roughness = spaceRoughness.GetTextureID();
    matInfo.ao = spaceAO.GetTextureID();
    matInfo.height = spaceHeight.GetTextureID();

    MaterialInstance material;
    material.CreateMaterialInstance(L"DefaultMaterial", matInfo);

    /*
    {
        auto ent = CreateEntity("DxCube");
        auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCube");
        sm.SetMaterialInstance(material);
        auto& trans = ent.GetComponent<TransformComponent>();
        trans.scale = { 10.f, 10.f, 10.f };
        trans.pos = { 0.f, 0.f, 0.f };
    }
    */
    for (size_t i = 0; i < 100; i++)
    {
    {
        auto ent = CreateEntity("DxCube");
        auto& sm = ent.AddComponent<MeshComponent>(L"DefaultSphere");
        sm.SetMaterialInstance(material);
        auto& trans = ent.GetComponent<TransformComponent>();
        trans.scale = { 2.f, 2.f, 2.f };
        trans.pos = {  -50.f + float(rand() % 100),float(rand() % 10), -50.f + float(rand() % 100) };
        trans.rot = DirectX::XMQuaternionRotationAxis({ 0.f, 1.f, 0.f }, XMConvertToRadians(45.f));
    }
    }
    {
    {
        auto ent = CreateEntity("testSphere");
        auto& sm = ent.AddComponent<MeshComponent>(L"DefaultSphere");
        sm.SetMaterialInstance(material);
        auto& trans = ent.GetComponent<TransformComponent>();
        trans.scale = { 2.f, 2.f, 2.f };
        trans.pos = { 3.f, 10.f, 0.f };       
    }
    }
    
	return true;
}

void SponzaExe::UnloadContent()
{
}

void SponzaExe::OnUpdate(UpdateEventArgs& e)
{
	Game::OnUpdate(e);
}
