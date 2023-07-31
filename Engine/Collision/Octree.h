#pragma once
#include <BoundingVolumes.h>
#include <vector>
#include <entt/entt.hpp>

struct OctreeEntity : public IBounded
{
    OctreeEntity(const AABB& aabb, const entt::entity entity)
        :aabb(aabb), entity(entity){};

    virtual const AABB& GetBounds() const override { return aabb; }

private:   
    AABB aabb;
    entt::entity entity;
};

template <typename T>
class Octree 
{
    static_assert(std::is_base_of<IBounded, T>::value, "Type T must derive from IBounded");

private:
    std::vector<Octree> children;
    std::vector<T> items;
    int depth;
    AABB bounds;
    const static int MAX_DEPTH = 5;
    const static int MAX_ITEMS = 5;

    void Split() 
    {
        if (!children.empty()) return;

        XMVECTOR size = (bounds.max - bounds.min) * 0.5f;
        XMVECTOR offset = size * 0.5f;

        for (int i = 0; i < 8; ++i) 
        {
            XMVECTOR childMin = bounds.min + XMVectorMultiply(offset, XMVectorSet((i & 1) ? 1 : 0, (i & 2) ? 1 : 0, (i & 4) ? 1 : 0, 1));
            XMVECTOR childMax = childMin + size;
            children.emplace_back(AABB(childMin, childMax), depth + 1);
        }

        for (T& item : items) 
        {
            for (Octree& child : children) 
            {
                child.Insert(item);
            }
        }

        items.clear();
    }

public:
    Octree(const AABB& bounds, int depth = 0) : bounds(bounds), depth(depth) {}

    void Insert(const T& item) 
    {
        auto itemBounds = item.GetBounds();

        if (children.empty() && (items.size() < MAX_ITEMS || depth >= MAX_DEPTH)) 
        {
            items.push_back(item);
            return;
        }

        if (children.empty()) 
        {
            Split();
        }

        for (Octree& child : children) 
        {
            if (child.bounds.Intersects(itemBounds)) 
            {
                child.Insert(item);
            }
        }
    }

    bool Remove(const T& item)
    {
        auto itemBounds = item.GetBounds();

        // First, try to remove from this node's items
        auto it = std::find(items.begin(), items.end(), item);
        if (it != items.end())
        {
            items.erase(it);
            return true;
        }

        // If not found, recurse into children
        for (Octree& child : children)
        {
            if (child.bounds.Intersects(itemBounds))
            {
                if (child.Remove(item)) return true;
            }
        }

        return false; // Not found in this subtree
    }

    void Query(const Frustum& frustum, std::vector<T>& result) const 
    {
        if (!frustum.Intersects(bounds)) return;

        for (const T& item : items) 
        {
            result.push_back(item);
        }

        for (const Octree& child : children) 
        {
            child.Query(frustum, result);
        }
    }

    void Query(const AABB& queryBounds, std::vector<T>& result) const 
    {
        if (!queryBounds.Intersects(bounds)) return;

        for (const T& item : items) {
            if (queryBounds.Intersects(item.GetBounds())) 
            {
                result.push_back(item);
            }
        }

        for (const Octree& child : children) 
        {
            child.Query(queryBounds, result);
        }
    }

    void Move(const T& oldItem, const T& newItem) 
    {
        // Remove the old item
        Remove(oldItem);

        // Insert the updated item
        Insert(newItem);
    }
};