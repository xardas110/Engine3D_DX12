#include <EnginePCH.h>
#include <PhysicsSystem.h>


PhysicsSystem::PhysicsSystem(entt::registry* registry)
    :mRegistry(registry)
{
    mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mDefaultAllocatorCallback,
        mDefaultErrorCallback);

    if (!mFoundation)
        throw std::exception("PxCreateFoundation failed!");

    bool recordMemoryAllocations = true;

    mPvd = PxCreatePvd(*mFoundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
    mPvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation,
        PxTolerancesScale(), recordMemoryAllocations, mPvd);

    if (!mPhysics)
        throw std::exception("PxCreatePhysics failed!");

    if (!PxInitExtensions(*mPhysics, mPvd))
        throw std::exception("PxInitExtensions failed!");
}

PhysicsSystem::~PhysicsSystem()
{
    mPhysics->release();
    mFoundation->release();
}

void PhysicsSystem::OnUpdate(float deltatime)
{

}
