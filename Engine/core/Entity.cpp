#include <EnginePCH.h>
#include <Entity.h>
#include <Components.h>

Entity::Entity(entt::entity id, std::shared_ptr<Game> game)
	:id(id), pGame(game)
{
}

void Entity::AddChild(Entity& child)
{
	auto& relation = GetComponent<RelationComponent>();
	auto& childRelation = child.GetComponent<RelationComponent>();
	childRelation.SetParent(GetId());
	relation.AddChild(child.GetId());
}
