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
    {
        auto sponza = CreateStaticMesh("sponza", "Assets/Models/sponza/Sponza.fbx", 
            MeshFlags::ForceAlphaCutoff | MeshFlags::CustomTangent);
        auto& trans = sponza.GetComponent<TransformComponent>();
        trans.pos = { 0.f, 0.f, 0.f };
        trans.scale = { 0.01f, 0.01f, 0.01f };
    }

    return true;
}

void SponzaExe::UnloadContent()
{}

void SponzaExe::OnUpdate(UpdateEventArgs& e)
{
	Game::OnUpdate(e);
}
