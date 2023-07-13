#pragma once
#include <d3d12.h>
#include <DirectXMath.h>

inline D3D12_CLEAR_VALUE ClearValue(const DXGI_FORMAT format, const DirectX::XMFLOAT4& color)
{
	D3D12_CLEAR_VALUE aoMetallicHeightClearValue;
	aoMetallicHeightClearValue.Format = format;
	aoMetallicHeightClearValue.Color[0] = color.x;
	aoMetallicHeightClearValue.Color[1] = color.y;
	aoMetallicHeightClearValue.Color[2] = color.z;
	aoMetallicHeightClearValue.Color[3] = color.w;

	return aoMetallicHeightClearValue;
}

