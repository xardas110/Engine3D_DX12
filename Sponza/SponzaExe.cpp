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
    m_Camera.set_LookAt({ -13.f, 3.5f, 2.8f }, {0.f, 0.f, 0.f}, {0.f, 1.f, 0.f});
    m_CameraController.pitch = 19.f;
    m_CameraController.yaw = -230.f;

    {
        auto sponza = CreateStaticMesh("sponza", "Assets/Models/sponza/Sponza.fbx", 
            MeshFlags::ForceAlphaCutoff | MeshFlags::CustomTangent);
        auto& trans = sponza.GetComponent<TransformComponent>();
        trans.pos = { 0.f, 0.f, 0.f };
        trans.scale = { 0.01f, 0.01f, 0.01f };
    }

    return true;
}

void SponzaExe::OnUpdate(UpdateEventArgs& e)
{
	Game::OnUpdate(e);   
}

void SponzaExe::UnloadContent()
{}
