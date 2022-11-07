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
    auto ent = game->CreateEntity("ent");
    auto ent2 = game->CreateEntity("ent2");
    auto ent3 = game->CreateEntity("ent3");
    auto ent4 = game->CreateEntity("ent4");
    auto ent5 = game->CreateEntity("ent5");
    auto ent6 = game->CreateEntity("ent6");

    auto ent10 = game->CreateEntity("Created a new entity 5677");


    for (int i = 0; i < 10; i++)
    {
        auto ent10 = game->CreateEntity("Created a new entity " + std::to_string(i));
    }

    ent2.GetComponent<TransformComponent>().pos = { 0.f, 20.f, 10.f, 0.f };
    ent5.GetComponent<TransformComponent>().pos = { 50.f, 25.f, 10.f, 0.f };

    ent.AddChild(ent2);
    ent.AddChild(ent3);
    ent3.AddChild(ent4);
    ent2.AddChild(ent6);
    ent2.AddChild(ent5);

    return true;
}


void Sponza::UnloadContent()
{
    std::cout << "Unloading content " << std::endl;
    game->ClearGameWorld();
    std::cout << "Clearing gameworld! " << std::endl;
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

