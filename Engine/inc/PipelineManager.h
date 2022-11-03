#pragma once

#include <RootSignature.h>
#include <array>

using PipelineRef = Microsoft::WRL::ComPtr<ID3D12PipelineState>;

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
		CameraCB,
		MaterialCB,
		Size
	};
}

struct PipelineData
{
	PipelineRef pipelineRef;
	RootSignature rootSignature;
};

using PipelineArray = std::array<PipelineData, Pipeline::Size>;

class PipelineManager
{
	friend class Application;
	friend class std::default_delete<PipelineManager>;

	PipelineManager();
	~PipelineManager();

	static std::unique_ptr<PipelineManager> CreateInstance();

private:
	void CreateGeometryMeshPSO();
	PipelineArray m_Pipelines;
};
