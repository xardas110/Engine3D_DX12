#pragma once

#include <entt/entt.hpp>
#include <Game.h>
#include <stdexcept>

class Entity
{
	friend class Game;
public:
	explicit Entity(entt::entity id, std::shared_ptr<Game> game);

	template<typename T>
	T& GetComponent() const
	{
		return GetGame()->registry.get<T>(id);
	}

	template<typename T, typename... Args>
	T& AddComponent(Args&&... args)
	{
		return GetGame()->registry.emplace<T>(id, std::forward<Args>(args)...);
	}

	template<typename T>
	bool HasComponent() const
	{
		return GetGame()->registry.any_of<T>(id);
	}

	template<typename T>
	size_t RemoveComponent()
	{
		return GetGame()->registry.remove<T>(id);
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

private:
	std::shared_ptr<Game> GetGame() const
	{
		auto game = pGame.lock();
		if (!game)
		{
			throw std::runtime_error("Game object has been destroyed");
		}
		return game;
	}

	entt::entity id{ entt::null };
	std::weak_ptr<Game> pGame;
};