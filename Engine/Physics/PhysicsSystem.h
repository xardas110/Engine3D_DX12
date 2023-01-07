#pragma once
#include <pxPhysicsAPI.h>
#include <entt/entt.hpp>
#include <PhysicsActor.h>

#define PVD_HOST "127.0.0.1"

using namespace physx;

class PhysicsSystem
{
	friend class Game;
	friend class PhysicsDynamic;
	friend class PhysicsStatic;

	PhysicsSystem(entt::registry* registry);
	~PhysicsSystem();

	void OnUpdate(float deltatime);

	PxFoundation* mFoundation = NULL;
	PxPhysics* mPhysics = NULL;
	PxDefaultCpuDispatcher* mDispatcher = NULL;
	PxScene* mScene = NULL;
	PxMaterial* mMaterial = NULL;
	PxPvd* mPvd = NULL;
	PxDefaultAllocator	mDefaultAllocatorCallback;
	PxDefaultErrorCallback	mDefaultErrorCallback;

	entt::registry* mRegistry;
};