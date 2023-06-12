#include <EnginePCH.h>
#include <PhysicsActor.h>
#include <Game.h>


PhysicsStatic::PhysicsStatic(PxRigidStatic* pxStatic)
	:ptr(pxStatic)
{
}

PhysicsDynamic::PhysicsDynamic(PxRigidDynamic* pxDynamic)
	:ptr(pxDynamic)
{
}
