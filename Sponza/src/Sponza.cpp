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
    std::cout << "Sponza xcv content " << std::endl;
    return true;
}


void Sponza::UnloadContent()
{
}

void Sponza::OnUpdate(UpdateEventArgs& e)
{
}

