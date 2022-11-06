#include <Sponza.h>

#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <Helpers.h>
#include <Window.h>

#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <Entity.h>

using namespace DirectX;

#include <algorithm> // For std::min and std::max.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#include <iostream>

#include <Components.h>

extern "C" __declspec(dllexport) GameMode* CALLBACK Entry(Game* gameEntry)
{
    return new Sponza(gameEntry);
}

Sponza::Sponza(Game* game)
    : GameMode(game)
{
}

Sponza::~Sponza()
{
}

bool Sponza::LoadContent()
{
    std::cout << "test1000 " << std::endl;

    auto ent = game->CreateEntity("Parent1");
    auto& tag = ent.GetComponent<TagComponent>();

    auto ent2 = game->CreateEntity("Child1");
    ent.AddChild(ent2);

    auto ent3 = game->CreateEntity("ent1");

    auto ent4 = game->CreateEntity("ent2");

    auto ent5 = game->CreateEntity("ent4");

    auto ent6 = game->CreateEntity("ent6");

    return true;
}


void Sponza::UnloadContent()
{
}

void Sponza::OnUpdate(UpdateEventArgs& e)
{
}

void Sponza::OnGui(RenderEventArgs& e)
{
    //std::cout << "OnGui running " << std::endl;
   // ImGui::Begin("Sponza");
   // ImGui::Text("Sponza mode");
   // ImGui::End();
}

