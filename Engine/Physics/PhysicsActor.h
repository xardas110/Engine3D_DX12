#pragma once
#include <pxPhysicsAPI.h>

using namespace physx;

struct PhysicsStatic
{
	PhysicsStatic(PxRigidStatic* pxStatic);
	PxRigidStatic* ptr;
};

struct PhysicsDynamic
{
	PhysicsDynamic(PxRigidDynamic* pxDynamic);
	PxRigidDynamic* ptr;
};

