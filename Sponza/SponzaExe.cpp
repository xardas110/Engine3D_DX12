#include "SponzaExe.h"
#include <iostream>
#include <Components.h>
#include <entity.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <gtest/gtest.h>

SponzaExe::SponzaExe(const std::wstring& name, int width, int height, bool vSync)
	:Game(name, width, height, vSync)
{
    srand(time(nullptr));
}

SponzaExe::~SponzaExe()
{}

bool SponzaExe::LoadContent()
{
    auto CQ = Application::Get().GetCommandQueue();
    auto CL = CQ->GetCommandList();
    auto RTCL = CQ->GetCommandList();

    {
        auto sponza = CreateEntity("sponza");
        auto& sm = sponza.AddComponent<StaticMeshComponent>(*CL, RTCL, "Assets/Models/sponza/Sponza.fbx",
            MeshImport::ForceAlphaCutoff | MeshImport::CustomTangent); //| MeshImport::IgnoreUserMaterial);
        auto& trans = sponza.GetComponent<TransformComponent>();
        trans.pos = { 0.f, 0.f, 0.f };
        trans.scale = { 0.01f, 0.01f, 0.01f };
        //trans.rot = DirectX::XMQuaternionRotationRollPitchYaw(0, DirectX::XMConvertToRadians(90.f), 0.f);
    }

    CQ->ExecuteCommandList(CL);
    CQ->ExecuteCommandList(RTCL);

    return true;
}

void SponzaExe::UnloadContent()
{}

void SponzaExe::OnUpdate(UpdateEventArgs& e)
{
	Game::OnUpdate(e);
}
