#pragma once
#include <pxPhysicsAPI.h>

using namespace physx;

struct PhysicsStatic
{
	PhysicsStatic(class Game* game, PxRigidStatic* pxStatic);
	PxRigidStatic* ptr;
};

struct PhysicsDynamic
{
	PhysicsDynamic(class Game* game, PxRigidDynamic* pxDynamic);
	PxRigidDynamic* ptr;
};

