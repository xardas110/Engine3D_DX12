#pragma once

#include <RootSignature.h>

using PipelineRef = Microsoft::WRL::ComPtr<ID3D12PipelineState>;

namespace Pipeline
{
	enum Type
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

using PipelineArray = std::array<PipelineData, Pipeline::Type::Size>;

class PipelineManager
{
	PipelineManager();
	~PipelineManager();

private:
	void CreateGeometryPSO();
	PipelineArray m_Pipelines;
};
