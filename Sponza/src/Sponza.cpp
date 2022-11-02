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

Sponza::Sponza(const std::wstring& name, int width, int height, bool vSync)
    : super(name, width, height, vSync)
{
}

Sponza::~Sponza()
{
}

bool Sponza::LoadContent()
{
    return true;
}

void Sponza::OnResize(ResizeEventArgs& e)
{
    super::OnResize(e);
}

void Sponza::UnloadContent()
{
}

void Sponza::OnUpdate(UpdateEventArgs& e)
{
    super::OnUpdate(e);
}

void Sponza::OnRender(RenderEventArgs& e)
{
    super::OnRender(e);
}

void Sponza::OnKeyPressed(KeyEventArgs& e)
{
    super::OnKeyPressed(e);
}

void Sponza::OnKeyReleased(KeyEventArgs& e)
{
    super::OnKeyReleased(e);
}

void Sponza::OnMouseMoved(MouseMotionEventArgs& e)
{
    super::OnMouseMoved(e);
}

void Sponza::OnMouseWheel(MouseWheelEventArgs& e)
{
  
}
