#include <EnginePCH.h>

#include <Application.h>
#include <Game.h>
#include <Window.h>
#include <Entity.h>
#include <Components.h>

// Clamp a value between a min and max range.
template<typename T>
constexpr const T& clamp(const T& val, const T& min = T(0), const T& max = T(1))
{
    return val < min ? min : val > max ? max : val;
}

Game::Game( const std::wstring& name, int width, int height, bool vSync )
    : m_Name( name )
    , m_Width( width )
    , m_Height( height )
    , m_vSync( vSync )
{
    XMVECTOR cameraPos = XMVectorSet(0, 5, -20, 1);
    XMVECTOR cameraTarget = XMVectorSet(0, 5, 0, 1);
    XMVECTOR cameraUp = XMVectorSet(0, 1, 0, 0);

    m_Camera.set_LookAt(cameraPos, cameraTarget, cameraUp);
    m_Camera.set_Projection(45.0f, width / (float)height, 0.1f, 100.0f);
}

Game::~Game()
{
    assert(!m_pWindow && "Use Game::Destroy() before destruction.");
}

bool Game::Initialize()
{
    // Check for DirectX Math library support.
    if (!DirectX::XMVerifyCPUSupport())
    {
        MessageBoxA(NULL, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    m_pWindow = Application::Get().CreateRenderWindow(m_Name, m_Width, m_Height, m_vSync);
    m_pWindow->RegisterCallbacks(shared_from_this());
    m_pWindow->Show();

    StaticMesh temp;

    auto* smm = Application::Get().GetAssetManager();
    smm->LoadStaticMesh("Assets/Models/crytek-sponza-noflag/sponza.dae", temp);

    return true;
}

void Game::Destroy()
{
    Application::Get().DestroyWindow(m_pWindow);
    m_pWindow.reset();
}

void Game::SetRenderCamera(Camera* camera)
{
    m_RenderCamera = camera;
}

const Camera* Game::GetRenderCamera() const
{
    return m_RenderCamera;
}

Entity Game::CreateEntity(const std::string& tag)
{
    Entity ent(registry.create(), shared_from_this());
    ent.AddComponent<TransformComponent>();
    ent.AddComponent<TagComponent>(tag);
    ent.AddComponent<RelationComponent>();
    return ent;
}

void Game::OnUpdate(UpdateEventArgs& e)
{
    // Update the camera.
    float speedMultipler = (m_Shift ? 16.0f : 4.0f);

    XMVECTOR cameraTranslate = XMVectorSet(m_Right - m_Left, 0.0f, m_Forward - m_Backward, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
    XMVECTOR cameraPan = XMVectorSet(0.0f, m_Up - m_Down, 0.0f, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
    m_Camera.Translate(cameraTranslate, Space::Local);
    m_Camera.Translate(cameraPan, Space::Local);

    XMVECTOR cameraRotation = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_Pitch), XMConvertToRadians(m_Yaw), 0.0f);
    m_Camera.set_Rotation(cameraRotation);

    XMMATRIX viewMatrix = m_Camera.get_ViewMatrix();
}

void Game::OnRender(RenderEventArgs& e)
{

}

void Game::OnKeyPressed(KeyEventArgs& e)
{
    if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        switch (e.Key)
        {
        case KeyCode::Escape:
            Application::Get().Quit(0);
            break;
        case KeyCode::F11:
            m_pWindow->ToggleFullscreen();
            break;
        case KeyCode::Enter:
            if (e.Alt)
            {
        case KeyCode::V:
            m_pWindow->ToggleVSync();
            break;
        case KeyCode::Up:
        case KeyCode::W:
            m_Forward = 1.0f;
            break;
        case KeyCode::Left:
        case KeyCode::A:
            m_Left = 1.0f;
            break;
        case KeyCode::Down:
        case KeyCode::S:
            m_Backward = 1.0f;
            break;
        case KeyCode::Right:
        case KeyCode::D:
            m_Right = 1.0f;
            break;
        case KeyCode::Q:
            m_Down = 1.0f;
            break;
        case KeyCode::E:
            m_Up = 1.0f;
            break;
        case KeyCode::ShiftKey:
            m_Shift = true;
            break;
            }
        }
    }
}

void Game::OnKeyReleased(KeyEventArgs& e)
{
    if (!ImGui::GetIO().WantCaptureKeyboard)
    {
        switch (e.Key)
        {
        case KeyCode::Enter:
            if (e.Alt)
            {
        case KeyCode::Up:
        case KeyCode::W:
            m_Forward = 0.0f;
            break;
        case KeyCode::Left:
        case KeyCode::A:
            m_Left = 0.0f;
            break;
        case KeyCode::Down:
        case KeyCode::S:
            m_Backward = 0.0f;
            break;
        case KeyCode::Right:
        case KeyCode::D:
            m_Right = 0.0f;
            break;
        case KeyCode::Q:
            m_Down = 0.0f;
            break;
        case KeyCode::E:
            m_Up = 0.0f;
            break;
        case KeyCode::ShiftKey:
            m_Shift = false;
            break;
            }
        }
    }
}

void Game::OnMouseMoved(class MouseMotionEventArgs& e)
{
    // By default, do nothing.
    const float mouseSpeed = 0.1f;
    if (!ImGui::GetIO().WantCaptureMouse)
    {
        if (e.RightButton)
        {
            m_Pitch += e.RelY * mouseSpeed;

            m_Pitch = clamp(m_Pitch, -90.0f, 90.0f);

            m_Yaw += e.RelX * mouseSpeed;
        }
    }
}

void Game::OnMouseButtonPressed(MouseButtonEventArgs& e)
{
    // By default, do nothing.
}

void Game::OnMouseButtonReleased(MouseButtonEventArgs& e)
{
    // By default, do nothing.
}

void Game::OnMouseWheel(MouseWheelEventArgs& e)
{
    // By default, do nothing.
}

void Game::OnResize(ResizeEventArgs& e)
{
    m_Width = e.Width;
    m_Height = e.Height;

    if (m_Width != e.Width || m_Height != e.Height)
    {
        float fov = m_Camera.get_FoV();
        float aspectRatio = m_Width / (float)m_Height;
        m_Camera.set_Projection(fov, aspectRatio, 0.1f, 100.0f);
    }
}

void Game::OnWindowDestroy()
{
    // If the Window which we are registered to is 
    // destroyed, then any resources which are associated 
    // to the window must be released.
    UnloadContent();
}

void Game::ClearGameWorld()
{
    registry.clear();
    registry = entt::registry();
}
