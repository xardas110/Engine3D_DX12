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

    {
        auto BistroInter = CreateEntity("BistroInterior");
        auto& sm = BistroInter.AddComponent<StaticMeshComponent>("Assets/Models/Bistro/BistroInterior.fbx",
            MeshImport::BaseColorAlpha | MeshImport::AO_Rough_Metal_As_Spec_Tex | MeshImport::InvertMaterialNormals | MeshImport::ForceAlphaCutoff | MeshImport::AssimpTangent);
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
        trans.rot = DirectX::XMQuaternionRotationRollPitchYaw(DirectX::XMConvertToRadians(90.f), DirectX::XMConvertToRadians(90.f), 0.f);
    }   
    {
        auto mercedes = CreateEntity("Mercedes");
        auto& sm = mercedes.AddComponent<StaticMeshComponent>("Assets/Models/mercedes/scene.gltf", MeshImport::ForceAlphaBlend);
        auto& trans = mercedes.GetComponent<TransformComponent>();
        trans.rot = DirectX::XMQuaternionRotationRollPitchYaw(DirectX::XMConvertToRadians(90.f), DirectX::XMConvertToRadians(90.f), 0.f);
        trans.pos = { 1.f, 0.23f, 12.f };
        trans.scale = { 1.f, 1.f, 1.f };
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
