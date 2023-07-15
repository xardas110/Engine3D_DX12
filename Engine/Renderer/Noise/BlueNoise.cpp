#include <EnginePCH.h>
#include <Noise/BlueNoise.h>
#include <CommandList.h>
#include <CommandQueue.h>
#include <Application.h>

void BlueNoise::LoadTextures(std::shared_ptr<CommandList>& commandList)
{
    commandList->LoadTextureFromFile(scrambling, L"Assets/Textures/BlueNoise/scrambling_ranking_128x128_2d_1spp.png");
    commandList->LoadTextureFromFile(sobol, L"Assets/Textures/BlueNoise/sobol_256_4d.png");

    auto device = Application::Get().GetDevice();

    D3D12_SHADER_RESOURCE_VIEW_DESC desc{};
    desc.Format = DXGI_FORMAT_R8G8B8A8_UINT;
    desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.Texture2D.MipLevels = 1;
    desc.Texture2D.MostDetailedMip = 0;

    device->CreateShaderResourceView(
        scrambling.GetD3D12Resource().Get(), &desc,
        heap.SetHandle(DENOISER_TEX_SCRAMBLING));

    device->CreateShaderResourceView(
        sobol.GetD3D12Resource().Get(), &desc,
        heap.SetHandle(DENOISER_TEX_SOBOL));
}
