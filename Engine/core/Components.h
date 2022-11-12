#pragma once
#include <Transform.h>
#include <Mesh.h>

using TransformComponent = Transform;
using StaticMeshComponent = StaticMesh;
using MeshComponent = Primitives::Type;
using TextureComponent = TextureWrapper;

struct TagComponent
{
	TagComponent(const std::string& tag) :tag(tag) {}

	const std::string& GetTag() const
	{
		return tag;
	}
private:
	std::string tag = "";
};

struct RelationComponent
{
	friend class Game;
	friend class Entity;

	~RelationComponent()
	{
		children.clear();
	}

	bool HasParent() const
	{
		return parent != entt::null;
	}

	bool HasChildren() const
	{
		return !children.empty();
	}

	entt::entity GetParent() const
	{
		return parent;
	}

	const std::vector<entt::entity>& GetChildren() const
	{
		return children;
	}
private:

	void SetParent(entt::entity parent)
	{
		this->parent = parent;
	}

	void AddChild(entt::entity child)
	{
		for (size_t i = 0; i < children.size(); i++)
		{
			if (child == children[i]) return;
		}
		children.emplace_back(child);
	}

	entt::entity parent{ entt::null };
	std::vector<entt::entity> children;
};