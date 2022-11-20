#include <EnginePCH.h>

#include <GUI.h>

#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <RenderTarget.h>
#include <RootSignature.h>
#include <Texture.h>
#include <Window.h>

#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

#include <StaticDescriptorHeap.h>

// Root parameters for the ImGui root signature.
enum RootParameters
{
    MatrixCB,           // cbuffer vertexBuffer : register(b0)
    FontTexture,        // Texture2D texture0 : register(t0);
    NumRootParameters
};

//--------------------------------------------------------------------------------------
// Get surface information for a particular format
//--------------------------------------------------------------------------------------
void GetSurfaceInfo(
    _In_ size_t width,
    _In_ size_t height,
    _In_ DXGI_FORMAT fmt,
    size_t* outNumBytes,
    _Out_opt_ size_t* outRowBytes,
    _Out_opt_ size_t* outNumRows );

GUI::GUI()
    : m_pImGuiCtx( nullptr )
{}

GUI::~GUI()
{
    Destroy();
}

bool GUI::Initialize( std::shared_ptr<Window> window )
{
    m_Window = window;

    m_pImGuiCtx = ImGui::CreateContext();
    ImGui::SetCurrentContext( m_pImGuiCtx );

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    if ( !m_Window || !ImGui_ImplWin32_Init( m_Window->GetWindowHandle() ) )
    {
        return false;
    }

    auto& device = Application::Get().GetDevice();

    ImGui_ImplDX12_Init(device.Get(), 3,
        DXGI_FORMAT_R8G8B8A8_UNORM, m_Heap.heap.Get(),
        m_Heap.heap->GetCPUDescriptorHandleForHeapStart(),
        m_Heap.heap->GetGPUDescriptorHandleForHeapStart());

    return true;
}

void GUI::NewFrame()
{
    ImGui::SetCurrentContext( m_pImGuiCtx );

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();

    ImGui::NewFrame();   
}

void GUI::Render( std::shared_ptr<CommandList> commandList, const RenderTarget& renderTarget)
{
    ImGuiIO& io = ImGui::GetIO();  

    // Rendering
    ImGui::Render();
    commandList->SetRenderTarget(renderTarget);
    commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_Heap.heap.Get());

    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList->GetGraphicsCommandList().Get());

    // Update and Render additional Platform Windows
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(NULL, (void*)commandList->GetGraphicsCommandList().Get());
    }
}

void GUI::Destroy()
{
    if (m_Window)
    {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext(m_pImGuiCtx);
        m_pImGuiCtx = nullptr;
        m_Window.reset();
    }
}

//--------------------------------------------------------------------------------------
// Get surface information for a particular format
//--------------------------------------------------------------------------------------
void GetSurfaceInfo(
    _In_ size_t width,
    _In_ size_t height,
    _In_ DXGI_FORMAT fmt,
    size_t* outNumBytes,
    _Out_opt_ size_t* outRowBytes,
    _Out_opt_ size_t* outNumRows )
{
    size_t numBytes = 0;
    size_t rowBytes = 0;
    size_t numRows = 0;

    bool bc = false;
    bool packed = false;
    bool planar = false;
    size_t bpe = 0;
    switch ( fmt )
    {
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            bc = true;
            bpe = 8;
            break;

        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            bc = true;
            bpe = 16;
            break;

        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_YUY2:
            packed = true;
            bpe = 4;
            break;

        case DXGI_FORMAT_Y210:
        case DXGI_FORMAT_Y216:
            packed = true;
            bpe = 8;
            break;

        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_420_OPAQUE:
            planar = true;
            bpe = 2;
            break;

        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
            planar = true;
            bpe = 4;
            break;
    }

    if ( bc )
    {
        size_t numBlocksWide = 0;
        if ( width > 0 )
        {
            numBlocksWide = std::max<size_t>( 1, ( width + 3 ) / 4 );
        }
        size_t numBlocksHigh = 0;
        if ( height > 0 )
        {
            numBlocksHigh = std::max<size_t>( 1, ( height + 3 ) / 4 );
        }
        rowBytes = numBlocksWide * bpe;
        numRows = numBlocksHigh;
        numBytes = rowBytes * numBlocksHigh;
    }
    else if ( packed )
    {
        rowBytes = ( ( width + 1 ) >> 1 ) * bpe;
        numRows = height;
        numBytes = rowBytes * height;
    }
    else if ( fmt == DXGI_FORMAT_NV11 )
    {
        rowBytes = ( ( width + 3 ) >> 2 ) * 4;
        numRows = height * 2; // Direct3D makes this simplifying assumption, although it is larger than the 4:1:1 data
        numBytes = rowBytes * numRows;
    }
    else if ( planar )
    {
        rowBytes = ( ( width + 1 ) >> 1 ) * bpe;
        numBytes = ( rowBytes * height ) + ( ( rowBytes * height + 1 ) >> 1 );
        numRows = height + ( ( height + 1 ) >> 1 );
    }
    else
    {
        size_t bpp = BitsPerPixel( fmt );
        rowBytes = ( width * bpp + 7 ) / 8; // round up to nearest byte
        numRows = height;
        numBytes = rowBytes * height;
    }

    if ( outNumBytes )
    {
        *outNumBytes = numBytes;
    }
    if ( outRowBytes )
    {
        *outRowBytes = rowBytes;
    }
    if ( outNumRows )
    {
        *outNumRows = numRows;
    }
}

