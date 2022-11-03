#pragma once

#include <DirectXMath.h>

struct Transform
{
	void SetTransform(DirectX::XMMATRIX& transform)
	{
		DirectX::XMMatrixDecompose(&scale, &rot, &pos, transform);
	}

	DirectX::XMMATRIX GetTransform() const
	{
		return DirectX::XMMatrixScalingFromVector(scale) 
			* DirectX::XMMatrixRotationQuaternion(rot) 
			* DirectX::XMMatrixTranslationFromVector(pos);
	}

	DirectX::XMVECTOR pos{0.f, 0.f, 0.f, 0.f};
	DirectX::XMVECTOR scale{ 1.f, 1.f, 1.f, 0.f };
	DirectX::XMVECTOR rot{DirectX::XMQuaternionIdentity()};
};