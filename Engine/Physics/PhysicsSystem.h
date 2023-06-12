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
	
	PxDefaultCpuDispatcher* mDispatcher = NULL;
	
	PxPvd* mPvd = NULL;
	PxDefaultAllocator	mAllocatorCallback;
	PxDefaultErrorCallback	mErrorCallback;
	PxTolerancesScale mToleranceScale;
	physx::PxPvdTransport* transport{ nullptr };


	entt::registry* mRegistry;

public:
	PxPhysics* mPhysics = NULL;
	PxScene* mScene = NULL;

};