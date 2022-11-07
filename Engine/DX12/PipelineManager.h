#pragma once

#include <RootSignature.h>
#include <array>

namespace Pipeline
{
	enum
	{
		GeometryMesh,
		Size
	};
}

namespace GeometryMeshRootParam
{
	enum
	{
		MatCB,
		MaterialCB,
		Textures,
		Size
	};
}

using PipelineRef = Microsoft::WRL::ComPtr<ID3D12PipelineState>;

struct PipelineData
{
	PipelineRef pipelineRef;
	RootSignature rootSignature;
};

using PipelineArray = std::array<PipelineData, Pipeline::Size>;

class PipelineManager
{
	friend class Application;
	friend class DeferredRenderer;
	friend class std::default_delete<PipelineManager>;

	PipelineManager();
	~PipelineManager();

	static std::unique_ptr<PipelineManager> CreateInstance();

private:
	void CreateGeometryMeshPSO();
	PipelineArray m_Pipelines;
};
