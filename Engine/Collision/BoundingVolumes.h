#pragma once

#include <DirectXMath.h>

class AABB;
class Frustum;

class IBounded
{
public:
    virtual const AABB& GetBounds() const = 0;
};

class BoundingVolume 
{
public:
    virtual ~BoundingVolume() = default;
    virtual bool Intersects(const BoundingVolume& other) const = 0;
};

class AABB : public BoundingVolume 
{
public:
    DirectX::XMVECTOR min;
    DirectX::XMVECTOR max;

    AABB(const DirectX::XMVECTOR& min, const DirectX::XMVECTOR& max) 
    {
        this->min = min;
        this->max = max;
    };

    bool IntersectsAABB(const AABB& other) const 
    {
        if (DirectX::XMVectorGetX(max) < DirectX::XMVectorGetX(other.min) || DirectX::XMVectorGetX(min) > DirectX::XMVectorGetX(other.max)) return false;
        if (DirectX::XMVectorGetY(max) < DirectX::XMVectorGetY(other.min) || DirectX::XMVectorGetY(min) > DirectX::XMVectorGetY(other.max)) return false;
        if (DirectX::XMVectorGetZ(max) < DirectX::XMVectorGetZ(other.min) || DirectX::XMVectorGetZ(min) > DirectX::XMVectorGetZ(other.max)) return false;
        return true;
    }

    virtual bool Intersects(const BoundingVolume& other) const override; 
};

class Frustum : public BoundingVolume 
{
private:
    DirectX::XMVECTOR planes[6];

public:
    Frustum(const DirectX::XMMATRIX& viewProjection)
    {
        DirectX::XMVECTOR row1 = viewProjection.r[0];
        DirectX::XMVECTOR row2 = viewProjection.r[1];
        DirectX::XMVECTOR row3 = viewProjection.r[2];
        DirectX::XMVECTOR row4 = viewProjection.r[3];

        planes[0] = DirectX::XMPlaneNormalize(DirectX::XMVectorAdd(row4, row1));
        planes[1] = DirectX::XMPlaneNormalize(DirectX::XMVectorSubtract(row4, row1));
        planes[2] = DirectX::XMPlaneNormalize(DirectX::XMVectorSubtract(row4, row2));
        planes[3] = DirectX::XMPlaneNormalize(DirectX::XMVectorAdd(row4, row2));
        planes[4] = DirectX::XMPlaneNormalize(DirectX::XMVectorAdd(row4, row3));
        planes[5] = DirectX::XMPlaneNormalize(DirectX::XMVectorSubtract(row4, row3));
    }

    bool IntersectsAABB(const AABB& box) const 
    {
        for (int i = 0; i < 6; ++i) {
            DirectX::XMVECTOR planeNormal = DirectX::XMVectorSetW(planes[i], 0);
            DirectX::XMVECTOR positiveVertex = box.min;

            if (DirectX::XMVectorGetX(planeNormal) >= 0)
                positiveVertex = DirectX::XMVectorSetX(positiveVertex, DirectX::XMVectorGetX(box.max));
            if (DirectX::XMVectorGetY(planeNormal) >= 0)
                positiveVertex = DirectX::XMVectorSetY(positiveVertex, DirectX::XMVectorGetY(box.max));
            if (DirectX::XMVectorGetZ(planeNormal) >= 0)
                positiveVertex = DirectX::XMVectorSetZ(positiveVertex, DirectX::XMVectorGetZ(box.max));

            if (DirectX::XMVectorGetX(DirectX::XMPlaneDotCoord(planes[i], positiveVertex)) < 0)
                return false;
        }
        return true;
    }

    virtual bool Intersects(const BoundingVolume& other) const override 
    {
        if (const AABB* aabb = dynamic_cast<const AABB*>(&other))
        {
            return IntersectsAABB(*aabb);
        }
        return false;
    }
};

inline bool AABB::Intersects(const BoundingVolume& other) const
{
    if (const AABB* aabb = dynamic_cast<const AABB*>(&other))
    {
        return IntersectsAABB(*aabb);
    }
    else if (const Frustum* frustum = dynamic_cast<const Frustum*>(&other))
    {
        return frustum->IntersectsAABB(*this);
    }
    return false;
};