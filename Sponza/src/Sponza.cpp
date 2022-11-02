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

Tutorial4::Tutorial4(const std::wstring& name, int width, int height, bool vSync)
    : super(name, width, height, vSync)
{
}

Tutorial4::~Tutorial4()
{
}

bool Tutorial4::LoadContent()
{
    return true;
}

void Tutorial4::OnResize(ResizeEventArgs& e)
{
    super::OnResize(e);
}

void Tutorial4::UnloadContent()
{
}

static double g_FPS = 0.0;

void Tutorial4::OnUpdate(UpdateEventArgs& e)
{
    super::OnUpdate(e);
}

void Tutorial4::OnRender(RenderEventArgs& e)
{
    super::OnRender(e);
}

void Tutorial4::OnKeyPressed(KeyEventArgs& e)
{
    super::OnKeyPressed(e);
}

void Tutorial4::OnKeyReleased(KeyEventArgs& e)
{
    super::OnKeyReleased(e);
}

void Tutorial4::OnMouseMoved(MouseMotionEventArgs& e)
{
    super::OnMouseMoved(e);
}


void Tutorial4::OnMouseWheel(MouseWheelEventArgs& e)
{
  
}
