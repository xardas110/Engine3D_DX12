#pragma once

#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)

namespace GlobalRootSignatureParams {
    enum Value {
        OutputViewSlot = 0,
        AccelerationStructureSlot,
        SceneConstantSlot,
        VertexBuffersSlot,
        Count
    };
}

namespace LocalRootSignatureParams {
    enum Value {
        CubeConstantSlot = 0,
        Count
    };
}

struct CubeConstantBuffer
{
    DirectX::XMFLOAT4 albedo;
};

class Raytracing
{
	friend class DeferredRenderer;

	void Init();

	void CreateRaytracingInterfaces();
	void CreateRootSignature();
    void CreateRaytracingPipelineStateObject();

	Microsoft::WRL::ComPtr<ID3D12Device5> m_DxrDevice;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> m_DxrCommandlist;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracingLocalRootSignature;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracingGlobalRootSignature;

    CubeConstantBuffer m_CubeCB;
};