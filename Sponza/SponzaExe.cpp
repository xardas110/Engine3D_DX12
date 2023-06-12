#include "SponzaExe.h"
#include <iostream>
#include <Components.h>
#include <entity.h>
#include <CommandQueue.h>
#include <CommandList.h>

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
    
    /*
    {
        auto BistroInter = CreateEntity("BistroInterior");
        auto& sm = BistroInter.AddComponent<StaticMeshComponent>("Assets/Models/Bistro/BistroInterior.fbx",
            MeshImport::BaseColorAlpha | MeshImport::AO_Rough_Metal_As_Spec_Tex | MeshImport::InvertMaterialNormals | MeshImport::ForceAlphaBlend | MeshImport::AssimpTangent);
        auto& trans = BistroInter.GetComponent<TransformComponent>();
        trans.pos = { 0.f, 0.f, 0.f };
        trans.scale = { 0.01f, 0.01f, 0.01f };
        trans.rot = DirectX::XMQuaternionRotationRollPitchYaw(DirectX::XMConvertToRadians(90.f), DirectX::XMConvertToRadians(90.f), 0.f);
    }  
    {
        auto BistroExt = CreateEntity("BistroExterior");
        auto& sm = BistroExt.AddComponent<StaticMeshComponent>("Assets/Models/Bistro/BistroExterior.fbx",
            MeshImport::BaseColorAlpha | MeshImport::AO_Rough_Metal_As_Spec_Tex | MeshImport::InvertMaterialNormals
            | MeshImport::ForceAlphaCutoff | MeshImport::CustomTangent);
        auto& trans = BistroExt.GetComponent<TransformComponent>();
        trans.pos = { 0.f, 0.f, 0.f };
        trans.scale = { 0.01f, 0.01f, 0.01f };
        trans.rot = DirectX::XMQuaternionRotationRollPitchYaw(DirectX::XMConvertToRadians(90.f), DirectX::XMConvertToRadians(90.f), DirectX::XMConvertToRadians(0.f));
    }
      {
        auto mercedes = CreateEntity("Mercedes");
        auto& sm = mercedes.AddComponent<StaticMeshComponent>("Assets/Models/mercedes/scene.gltf", MeshImport::ForceAlphaBlend);
        auto& trans = mercedes.GetComponent<TransformComponent>();
        trans.rot = DirectX::XMQuaternionRotationRollPitchYaw(DirectX::XMConvertToRadians(90.f), DirectX::XMConvertToRadians(90.f), 0.f);
        trans.pos = { 1.f, 0.28f, 12.f };
        trans.scale = { 1.f, 1.f, 1.f };
    }
    */
    /*
    meshManager.CreateCube();
    meshManager.CreateSphere();

    {
        Material materialEmissive;
        materialEmissive.diffuse = { 1.f, 1.f, 1.f, 1.f };
        materialEmissive.emissive = { 0.f, 0.f, 0.f };
        materialEmissive.roughness = 0.0f;
        materialEmissive.metallic = 1.0f;

        auto materialID = MaterialInstance::CreateMaterial(L"Emissive Material", materialEmissive);

        MaterialInstance material;
        MaterialInfo matInfo;

        material.CreateMaterialInstance(L"DefaultMaterialInstance", matInfo);
        material.SetMaterial(materialID);
        material.SetFlags(INSTANCE_OPAQUE);

        auto ent = CreateEntity("Test Cube: ");
        auto& sm = ent.AddComponent<MeshComponent>(L"DefaultSphere");
        sm.SetMaterialInstance(material);
        auto& trans = ent.GetComponent<TransformComponent>();
        trans.scale = { 2.f, 2.f, 2.f };
        trans.pos = { -8.f, 1.f, 0.5f };
    }
   
    {
        auto mercedes = CreateEntity("Mercedes");
        auto& sm = mercedes.AddComponent<StaticMeshComponent>("Assets/Models/mercedes/scene.gltf", MeshImport::ForceAlphaBlend);
        auto& trans = mercedes.GetComponent<TransformComponent>();
        trans.rot = DirectX::XMQuaternionRotationRollPitchYaw(DirectX::XMConvertToRadians(90.f), DirectX::XMConvertToRadians(90.f), 0.f);
        trans.pos = { 0.f, 0.0f, 0.5f };
        trans.scale = { 1.f, 1.f, 1.f };
    }
    */

    {
        auto sponza = CreateEntity("sponza");
        auto& sm = sponza.AddComponent<StaticMeshComponent>("Assets/Models/sponza/Sponza.fbx",
            MeshImport::ForceAlphaCutoff | MeshImport::CustomTangent | MeshImport::IgnoreUserMaterial);
        auto& trans = sponza.GetComponent<TransformComponent>();
        trans.pos = { 0.f, 0.f, 0.f };
        trans.scale = { 0.01f, 0.01f, 0.01f };
        //trans.rot = DirectX::XMQuaternionRotationRollPitchYaw(0, DirectX::XMConvertToRadians(90.f), 0.f);
    }

    return true;

    /*
    {
        { //floor
            Material materialEmissive;
            materialEmissive.diffuse = { 1.f, 1.f, 1.f, 1.f };
            materialEmissive.emissive = { 0.f, 0.f, 0.f };
            materialEmissive.roughness = 0.0f;
            materialEmissive.metallic = 1.0f;

            auto materialID = MaterialInstance::CreateMaterial(L"Emissive Material", materialEmissive);

            MaterialInstance material;
            MaterialInfo matInfo;

            material.CreateMaterialInstance(L"DefaultMaterialInstance", matInfo);
            material.SetMaterial(materialID);
            material.SetFlags(INSTANCE_OPAQUE);

            {
                auto ent = CreateEntity("Test Cube: ");
                auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCube");
                sm.SetMaterialInstance(material);
                auto& trans = ent.GetComponent<TransformComponent>();
                trans.scale = { 2.f, 2.f, 2.f };
                trans.pos = { 10.f, 1.f, 0.f };

            }

            {
                auto ent = CreateEntity("Test Cube: ");
                auto& sm = ent.AddComponent<MeshComponent>(L"DefaultSphere");
                sm.SetMaterialInstance(material);
                auto& trans = ent.GetComponent<TransformComponent>();
                trans.scale = { 2.f, 2.f, 2.f };
                trans.pos = { -10.f, 1.f, 0.f };

            }
        }


    }
    */


    auto physX = m_PhysicsSystem.mPhysics;
    auto pScene = m_PhysicsSystem.mScene;
    auto mat = physX->createMaterial(0.5f, 0.5f, 0.6f);

    { //floor
        Material materialEmissive;
        materialEmissive.diffuse = { 1.f, 1.f, 1.f, 1.f };
        materialEmissive.emissive = { 0.f, 0.f, 0.f };
        materialEmissive.roughness = 0.5f;
        materialEmissive.metallic = 0.5f;

        auto materialID = MaterialInstance::CreateMaterial(L"Emissive Material", materialEmissive);

        MaterialInstance material;
        MaterialInfo matInfo;

        material.CreateMaterialInstance(L"DefaultMaterialInstance", matInfo);
        material.SetMaterial(materialID);
        material.SetFlags(INSTANCE_OPAQUE);

        {
            auto ent = CreateEntity("Test Cube: ");
            auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCube");
            sm.SetMaterialInstance(material);
            auto& trans = ent.GetComponent<TransformComponent>();
            trans.scale = { 100.f, 2.f, 100.f };

            auto t = PxTransform(PxVec3(0, 0, 0));

            PxRigidStatic* groundPlane = PxCreateStatic(*physX, t, PxBoxGeometry(50.f, 1.f, 50.f), *mat);
            pScene->addActor(*groundPlane);

            auto& body = ent.AddComponent<PhysicsStaticComponent>(groundPlane);

        }
    }
    {//walls
        Material material;
        material.diffuse = { 1.f, 1.f, 0.f, 1.f };
        material.emissive = { 0.f, 0.f, 0.f };
        material.roughness = 0.5f;
        material.metallic = 0.5f;

        auto materialID = MaterialInstance::CreateMaterial(L"WallMaterial", material);

        MaterialInstance materialInstance;
        MaterialInfo matInfo;

        materialInstance.CreateMaterialInstance(L"WallMaterialInstance", matInfo);
        materialInstance.SetMaterial(materialID);
        materialInstance.SetFlags(INSTANCE_OPAQUE);

        //outer walls
        {
            auto ent = CreateEntity("Test Cube: ");
            auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCube");
            sm.SetMaterialInstance(materialInstance);
            auto& trans = ent.GetComponent<TransformComponent>();
            trans.scale = { 2.f, 30.f, 100.f };
            trans.pos = { -50.f, 15.f, 0.f };

            auto t = PxTransform(PxVec3(-50.f, 15.f, 0.f));

            PxRigidStatic* shape = PxCreateStatic(*physX, t, PxBoxGeometry(1.f, 15.f, 50.f), *mat);
            pScene->addActor(*shape);

            auto& body = ent.AddComponent<PhysicsStaticComponent>(shape);

        }
        {
            auto ent = CreateEntity("Test Cube: ");
            auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCube");
            sm.SetMaterialInstance(materialInstance);
            auto& trans = ent.GetComponent<TransformComponent>();
            trans.scale = { 2.f, 30.f, 100.f };
            trans.pos = { 0.f, 15.f, 50.f };
            trans.rot = DirectX::XMQuaternionRotationRollPitchYaw(0.f, DirectX::XMConvertToRadians(90.f), 0.f);

            auto t = PxTransform(PxVec3(0.f, 15.f, 50.f));

            PxRigidStatic* shape = PxCreateStatic(*physX, t, PxBoxGeometry(50.f, 15.f, 1.f), *mat);
            pScene->addActor(*shape);

            auto& body = ent.AddComponent<PhysicsStaticComponent>(shape);
        }
        {
            auto ent = CreateEntity("Test Cube: ");
            auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCube");
            sm.SetMaterialInstance(materialInstance);
            auto& trans = ent.GetComponent<TransformComponent>();
            trans.scale = { 2.f, 30.f, 100.f };
            trans.pos = { 0.f, 15.f, -50.f };
            trans.rot = DirectX::XMQuaternionRotationRollPitchYaw(0.f, DirectX::XMConvertToRadians(90.f), 0.f);

            auto t = PxTransform(PxVec3(0, 15, -50));

            PxRigidStatic* shape = PxCreateStatic(*physX, t, PxBoxGeometry(50.f, 15.f, 1.f), *mat);
            pScene->addActor(*shape);

            auto& body = ent.AddComponent<PhysicsStaticComponent>(shape);
        }
    } 
    {
        Material material;
        material.diffuse = { 1.f, 1.f, 1.f, 1.f };
        material.emissive = { 0.f, 0.f, 0.f };
        material.roughness = 0.5f;
        material.metallic = 0.5f;

        auto materialID = MaterialInstance::CreateMaterial(L"CubeMaterial", material);

        MaterialInstance materialInstance;
        MaterialInfo matInfo;

        materialInstance.CreateMaterialInstance(L"CubeMaterialInstance", matInfo);
        materialInstance.SetMaterial(materialID);
        materialInstance.SetFlags(INSTANCE_OPAQUE);

        /*
        for (int i = 0 ; i < 100; i++)
        {
            auto ent = CreateEntity("Test Cube: ");
            auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCube");
            sm.SetMaterialInstance(materialInstance);
            auto& trans = ent.GetComponent<TransformComponent>();
            trans.scale = { 3.f, 3.f, 3.f };

            float x = rand() % 20 - 10;
            float y = rand() % 20 + 5000;
            float z = rand() % 20 - 10;

            trans.pos = { x, y, z };

            auto t = PxTransform(PxVec3(x, y, z));

            PxRigidDynamic* dynamic = PxCreateDynamic(*physX, t, PxSphereGeometry(1.5), *mat, 10.0f);
            dynamic->setAngularDamping(0.5f);
            dynamic->setLinearVelocity(PxVec3(0, -0, -0));
            pScene->addActor(*dynamic);

            auto& body = ent.AddComponent<PhysicsDynamicComponent>(dynamic);

        }     
        */
    }

    {
        Material material;
        material.diffuse = { 0.f, 0.f, 1.f, 1.f };
        material.emissive = { 0.f, 0.f, 0.f };
        material.roughness = 0.1f;
        material.metallic = 0.9f;

        auto materialID = MaterialInstance::CreateMaterial(L"CubeMaterial1", material);

        MaterialInstance materialInstance;
        MaterialInfo matInfo;

        materialInstance.CreateMaterialInstance(L"CubeMaterialInstance1", matInfo);
        materialInstance.SetMaterial(materialID);
        materialInstance.SetFlags(INSTANCE_OPAQUE);

        for (int i = 0; i < 100; i++)
        {
            auto ent = CreateEntity("Test Cube: ");
            auto& sm = ent.AddComponent<MeshComponent>(L"DefaultCube");
            sm.SetMaterialInstance(materialInstance);
            auto& trans = ent.GetComponent<TransformComponent>();
            trans.scale = { 3.f, 3.f, 3.f };

            float x = rand() % 20 - 10;
            float y = rand() % 20 + 5000;
            float z = rand() % 20 - 10;

            trans.pos = { x, y, z };

            auto t = PxTransform(PxVec3(x, y, z));

            PxRigidDynamic* dynamic = PxCreateDynamic(*physX, t, PxBoxGeometry(1.5f, 1.5f, 1.5f), *mat, 10.0f);
            dynamic->setAngularDamping(0.5f);
            dynamic->setLinearVelocity(PxVec3(0, -0, -0));
            pScene->addActor(*dynamic);

            auto& body = ent.AddComponent<PhysicsDynamicComponent>(dynamic);

        }
    }



}

void SponzaExe::UnloadContent()
{
}

void SponzaExe::OnUpdate(UpdateEventArgs& e)
{
	Game::OnUpdate(e);
}
