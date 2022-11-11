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


    void CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline);
    void SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, Microsoft::WRL::ComPtr<ID3D12RootSignature>* rootSig);

	Microsoft::WRL::ComPtr<ID3D12Device5> m_DxrDevice;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> m_DxrCommandlist;
    Microsoft::WRL::ComPtr<ID3D12StateObject> m_DxrStateObject;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracingLocalRootSignature;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_raytracingGlobalRootSignature;

    CubeConstantBuffer m_CubeCB;
};