#include <EnginePCH.h>
#include <PhysicsActor.h>
#include <Game.h>


PhysicsStatic::PhysicsStatic(Game* game, PxRigidStatic* pxStatic)
	:ptr(pxStatic)
{
	game->m_PhysicsSystem.mScene->addActor(*pxStatic);
}

PhysicsDynamic::PhysicsDynamic(Game* game, PxRigidDynamic* pxDynamic)
	:ptr(pxDynamic)
{
	game->m_PhysicsSystem.mScene->addActor(*pxDynamic);
}
