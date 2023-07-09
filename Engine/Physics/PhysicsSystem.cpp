#include <EnginePCH.h>
#include <PhysicsSystem.h>
#include <Components.h>


PhysicsSystem::PhysicsSystem(entt::registry* registry)
    :mRegistry(registry)
{
	mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mAllocatorCallback, mErrorCallback);

	if (!mFoundation)
	{
		std::cout << "Failed to create physics foundation" << std::endl;
		return;
	}

	mToleranceScale.length = 100.f;
	mToleranceScale.speed = 981;

	mPvd = PxCreatePvd(*mFoundation);
	transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	mPvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

	mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation,
		mToleranceScale, true, mPvd);

	if (!mPhysics)
	{
		std::cout << "Failed to create physics scene " << std::endl;
		return;
	}

	PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.f, -9.81f, 0.f);

	mDispatcher = PxDefaultCpuDispatcherCreate(32);
	sceneDesc.cpuDispatcher = mDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;

	mScene = mPhysics->createScene(sceneDesc);

	if (!mScene)
	{
		std::cout << "Failed to create physics scene " << std::endl;
		return;
	}

	PxPvdSceneClient* pvdClient = mScene->getScenePvdClient();

	if (pvdClient)
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	std::cout << "Successfully created physX" << std::endl;
}

PhysicsSystem::~PhysicsSystem()
{
    mPhysics->release();
    mFoundation->release();
}

void PhysicsSystem::OnUpdate(float deltatime)
{
	auto view = mRegistry->view<PhysicsDynamicComponent>();
	for (int i = 0; i < view.size(); i++)
	{
		auto entity = view[i];
		auto& body = view.get<PhysicsDynamicComponent>(entity);
		auto& trans = mRegistry->get<TransformComponent>(entity);

		PxTransform transform; transform.p = PxVec3(trans.pos.m128_f32[0], trans.pos.m128_f32[1], trans.pos.m128_f32[2]);
		transform.q = PxQuat(trans.rot.m128_f32[0], trans.rot.m128_f32[1], trans.rot.m128_f32[2], trans.rot.m128_f32[3]);
		body.ptr->setGlobalPose(transform);
	}

	mScene->simulate(deltatime);
	mScene->fetchResults(true);

	for (int i = 0; i < view.size(); i++)
	{
		auto entity = view[i];

		auto& body = view.get<PhysicsDynamicComponent>(entity);
		auto& trans = mRegistry->get<TransformComponent>(entity);

		PxTransform transform = body.ptr->getGlobalPose();

		trans.pos = { transform.p.x, transform.p.y, transform.p.z };
		trans.rot = { transform.q.x, transform.q.y, transform.q.z, transform.q.w };
	}
}
