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

    auto ent = game->registry.create();

    auto& trans = game->registry.emplace<TransformComponent>(ent);

    trans.pos = { 10.f, 0.f, 0.f, 0.f };

    std::cout << trans.pos.m128_f32[0] << std::endl;

    return true;
}


void Sponza::UnloadContent()
{
}

void Sponza::OnUpdate(UpdateEventArgs& e)
{
}

