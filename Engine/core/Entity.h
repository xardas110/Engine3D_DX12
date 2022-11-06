#pragma once

#include <entt/entt.hpp>
#include <Game.h>

class Entity
{
	friend class Game;
	
	entt::entity id{ entt::null };
	std::weak_ptr<Game> pGame;
public:
	Entity(entt::entity id, std::shared_ptr<Game> game);

	template<typename T>
	T& GetComponent()
	{
		auto game = pGame.lock(); assert(game);
		return game->registry.get<T>(id);
	}

	template<typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		auto game = pGame.lock(); assert(game);		
		return game->registry.emplace<T>(id, std::forward<Args>(args)...);
	}

	template<typename T>
	bool HasComponent()
	{
		auto game = pGame.lock(); assert(game);	
		return game->registry.any_of<T>(id);
	}

	template<typename T>
	size_t RemoveComponent()
	{
		auto game = pGame.lock(); assert(game);
		return game->registry.remove<T>(id);
	}

	entt::entity GetId() const
	{
		return id;
	}

	bool IsValid() const
	{
		auto game = pGame.lock(); assert(game);
		return game->registry.valid(id);
	}

	void AddChild(Entity& child);
};