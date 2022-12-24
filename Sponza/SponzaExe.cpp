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

    auto& meshManager = Application::Get().GetAssetManager()->m_MeshManager;
    auto& materialManager = Application::Get().GetAssetManager()->m_MaterialManager;
/*
    {
        auto bathRoom = CreateEntity("Simple Room");
        auto& sm = bathRoom.AddComponent<StaticMeshComponent>("Assets/Models/simple_room/simple_room.obj", MeshImport::HeightAsNormal);
        auto& trans = bathRoom.GetComponent<TransformComponent>();
        trans.pos = { 70.f, 0.f, 0.f };

    }
*/

    {
        auto mcLaren = CreateEntity("Mercedes");
        auto& sm = mcLaren.AddComponent<StaticMeshComponent>("Assets/Models/mercedes/scene.gltf", MeshImport::ForceAlphaBlend);
        auto& trans = mcLaren.GetComponent<TransformComponent>();
        trans.rot = DirectX::XMQuaternionRotationRollPitchYaw(DirectX::XMConvertToRadians(270.f), DirectX::XMConvertToRadians(90.f), 0.f);      
        trans.pos = { 0.f, 0.f, 0.f };
        trans.scale = { 2.f, 2.f, 2.f };
    } 

    /*
    {
        auto mcLaren = CreateEntity("Lambo");
        auto& sm = mcLaren.AddComponent<StaticMeshComponent>("Assets/Models/lambo/scene.gltf", MeshImport::None);
        auto& trans = mcLaren.GetComponent<TransformComponent>();
        trans.pos = { 10.f, 0.f, 0.f };
        trans.rot = DirectX::XMQuaternionRotationRollPitchYaw(0.f, DirectX::XMConvertToRadians(90.f), 0.f);
    }
  
  
    */
    /*
    {
        auto sponza = CreateEntity("sponza");
      
        sponza.AddComponent<StaticMeshComponent>("Assets/Models/sponza/sponza.fbx", 
            MeshImport::HeightAsNormal | MeshImport::AmbientAsMetallic | MeshImport::ForceAlphaCutoff);

        auto& trans = sponza.GetComponent<TransformComponent>();
        trans.scale = { 0.015f, 0.015f, 0.015f };
    }
     */
    /*
    {
        auto modelEnt = CreateEntity("Mirror");

        modelEnt.AddComponent<StaticMeshComponent>("Assets/Models/Mirrors/mirror.gltf",
            MeshImport::HeightAsNormal);

        auto& trans = modelEnt.GetComponent<TransformComponent>();
        trans.pos = { 0.f, 50.f, 0.f, 0.f };
        trans.scale = { 10.f, 10.f, 10.f, 0.f };
    }
    */
    /*
    */
/*
    meshManager.CreateCube();
    meshManager.CreateSphere();
   // meshManager.CreateTorus();
   // meshManager.CreateCone();
    
    std::cout << "game init" << std::endl;

    Material materialEmissive;
    materialEmissive.diffuse = { 0.f, 1.f, 0.f, 1.f };
    materialEmissive.emissive = { 0.f, 10.f, 0.f };
   
    auto materialID = MaterialInstance::CreateMaterial(L"DefaultEmissiveRed", materialEmissive);

    MaterialInstance material;
    MaterialInfo matInfo;
    material.CreateMaterialInstance(L"DefaultMaterialInstance", matInfo);
    material.SetMaterial(materialID);
    material.SetFlags(INSTANCE_OPAQUE);

    for (size_t i = 0; i < 20; i++)
    {
        auto ent = CreateEntity("testSphere: " + std::to_string(i));
        auto& sm = ent.AddComponent<MeshComponent>(L"DefaultSphere");
        sm.SetMaterialInstance(material);
        auto& trans = ent.GetComponent<TransformComponent>();
        trans.scale = { 2.f, 2.f, 2.f };
        float x = (rand() % 30) - 15;
        float y = (rand() % 15);
        float z = (rand() % 30) - 15;
        trans.pos = { x, y, z };
        lights.emplace_back((std::uint32_t)ent.GetId());
    }
    /*
    {
        auto ent = CreateEntity("DxCube");
        auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCube");
        sm.SetMaterialInstance(material);
        auto& trans = ent.GetComponent<TransformComponent>();
        trans.scale = { 10.f, 10.f, 10.f };
        trans.pos = { 10.f, 10.f, 0.f };
    }

    */
    /*
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
    material.CreateMaterialInstance(L"DefaultMaterialInstance", matInfo);
    material.SetMaterial(materialID);
   
    {
        auto ent = CreateEntity("DxCube");
        auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCube");
        sm.SetMaterialInstance(material);
        auto& trans = ent.GetComponent<TransformComponent>();
        trans.scale = { 10.f, 10.f, 10.f };
        trans.pos = { 0.f, 0.f, 0.f };
    }
    
    float spacing = 10.f;

    for (size_t i = 0; i < 10; i++)
    {
        {
            auto ent = CreateEntity("DxCube");
            auto& sm = ent.AddComponent<MeshComponent>(L"DefaultSphere");
            sm.SetMaterialInstance(material);
            auto& trans = ent.GetComponent<TransformComponent>();
            trans.scale = { 2.f, 2.f, 2.f };
            trans.pos = { -50.f + spacing * i, spacing, -5.f };
            //trans.rot = DirectX::XMQuaternionRotationAxis({ 0.f, 1.f, 0.f }, XMConvertToRadians(45.f));
        }
    }
    
    for (size_t i = 0; i < 10; i++)
    {
    {
        auto ent = CreateEntity("DxCube");
        auto& sm = ent.AddComponent<MeshComponent>(L"DefaultTorus");
        sm.SetMaterialInstance(material);
        auto& trans = ent.GetComponent<TransformComponent>();
        trans.scale = { 2.f, 2.f, 2.f };
        trans.pos = {  -50.f + spacing * i, spacing, -10.f };
        //trans.rot = DirectX::XMQuaternionRotationAxis({ 0.f, 1.f, 0.f }, XMConvertToRadians(45.f));
    }
    }
    for (size_t i = 0; i < 10; i++)
    {
        {
            auto ent = CreateEntity("DxCube");
            auto& sm = ent.AddComponent<MeshComponent>(L"DefaultSphere");
            sm.SetMaterialInstance(material);
            auto& trans = ent.GetComponent<TransformComponent>();
            trans.scale = { 2.f, 2.f, 2.f };
            trans.pos = { -50.f + spacing * i, 5.f, -10.f };
            //trans.rot = DirectX::XMQuaternionRotationAxis({ 0.f, 1.f, 0.f }, XMConvertToRadians(45.f));
        }
    }
    for (size_t i = 0; i < 10; i++)
    {
        {
            auto ent = CreateEntity("DxCube");
            auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCube");
            sm.SetMaterialInstance(material);
            auto& trans = ent.GetComponent<TransformComponent>();
            trans.scale = { 8.f, 8.f, 8.f };
            trans.pos = { -50.f + spacing * i, 15.f, -10.f };
            //trans.rot = DirectX::XMQuaternionRotationAxis({ 0.f, 1.f, 0.f }, XMConvertToRadians(45.f));
        }
    }
    for (size_t i = 0; i < 10; i++)
    {
        {
            auto ent = CreateEntity("DxCube");
            auto& sm = ent.AddComponent<MeshComponent>(L"DefaultSphere");
            sm.SetMaterialInstance(material);
            auto& trans = ent.GetComponent<TransformComponent>();
            trans.scale = { 5.f, 5.f, 5.f };
            trans.pos = { -50.f + spacing * i, spacing, -20.f };
            //trans.rot = DirectX::XMQuaternionRotationAxis({ 0.f, 1.f, 0.f }, XMConvertToRadians(45.f));
        }
    }

    for (size_t i = 0; i < 50; i++)
    {
        {
            auto ent = CreateEntity("DxCube");
            auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCube");
            sm.SetMaterialInstance(material);
            auto& trans = ent.GetComponent<TransformComponent>();
            trans.scale = { 5.f, 5.f, 5.f };
            trans.pos = { -50.f + float(rand() % 100),float(rand() % 10), -50.f + float(rand() % 100) };
            //trans.rot = DirectX::XMQuaternionRotationAxis({ 0.f, 1.f, 0.f }, XMConvertToRadians(rand() % 360));
        }
    }

    {
        auto ent = CreateEntity("DxCube");
        auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCube");
        sm.SetMaterialInstance(material);
        auto& trans = ent.GetComponent<TransformComponent>();
        trans.scale = { 50.f, 2.f, 50.f };
        trans.pos = { 0.f, 0.f, 0.f};
    }

    {
        auto ent = CreateEntity("DxCube");
        auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCube");
        sm.SetMaterialInstance(material);
        auto& trans = ent.GetComponent<TransformComponent>();
        trans.scale = { 50.f, 50.f, 2.f };
        trans.pos = { 0.f, 30.f, 0.f };
    }

    /*
    {
        auto ent = CreateEntity("DxCube");
        auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCube");
        sm.SetMaterialInstance(material);
        auto& trans = ent.GetComponent<TransformComponent>();
        trans.scale = { 50.f, 50.f, 2.f };
        trans.pos = { 0.f, 0.f, -25.f };
    }
    */
/*
    for (size_t i = 0; i < 20; i++)
    {
        {
            auto ent = CreateEntity("DxCube");
            auto& sm = ent.AddComponent<MeshComponent>(L"DefaultTorus");
            sm.SetMaterialInstance(material);
            auto& trans = ent.GetComponent<TransformComponent>();
            trans.scale = { 5.f, 5.f, 5.f };
            trans.pos = { -50.f + float(rand() % 100),float(rand() % 10), -50.f + float(rand() % 100) };
            //trans.rot = DirectX::XMQuaternionRotationAxis({ 0.f, 1.f, 0.f }, XMConvertToRadians(rand() % 360));
        }
    }

    for (size_t i = 0; i < 20; i++)
    {
        {
            auto ent = CreateEntity("DxCube");
            auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCone");
            sm.SetMaterialInstance(material);
            auto& trans = ent.GetComponent<TransformComponent>();
            trans.scale = { 5.f, 5.f, 5.f };
            trans.pos = { -50.f + float(rand() % 100),float(rand() % 10), -50.f + float(rand() % 100) };
            //trans.rot = DirectX::XMQuaternionRotationAxis({ 0.f, 1.f, 0.f }, XMConvertToRadians(rand() % 360));
        }
    }

    {

    }
    */


    auto& view = registry.view<TransformComponent, MeshComponent>();
    int i = 0;
    for (auto [entity, trans, mesh] : view.each())
    {
        initalPositions.push_back(trans);
    }

	return true;
}

void SponzaExe::UnloadContent()
{
}

void SponzaExe::OnUpdate(UpdateEventArgs& e)
{
	Game::OnUpdate(e);
/*
    static float startY = 0.f;
    static float endY = 10.f;
    static float currentY = 0;
    static int sign = 1;

    currentY += e.ElapsedTime * sign;

    if (currentY > endY) sign = -1;
    else if (currentY < startY) sign = 1;

    for (size_t i = 0; i < lights.size(); i++)
    {
        entt::entity ent = (entt::entity)lights[i];

        auto& trans = registry.get<TransformComponent>(ent);
        trans.pos.m128_f32[1] = currentY;
    }
    */
}
