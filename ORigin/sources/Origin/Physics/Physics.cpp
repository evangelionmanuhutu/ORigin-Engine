#include "pch.h"

#include "Physics.h"
#include "PhysXAPI.h"

namespace origin {

	std::unique_ptr<PhysicsAPI> Physics::m_PhysicsAPI;

	void Physics::Init()
	{
		PROFILER_PHYSICS();

		switch (PhysicsAPI::Current())
		{
		case PhysicsAPIType::Jolt: break;
		case PhysicsAPIType::PhysX: 
			m_PhysicsAPI = std::make_unique<PhysXAPI>();
			m_PhysicsAPI->Init();
			break;
		case PhysicsAPIType::None: break;
		}
	}

	void Physics::Shutdown()
	{
		PROFILER_PHYSICS();

		switch (PhysicsAPI::Current())
		{
		case PhysicsAPIType::Jolt: break;
		case PhysicsAPIType::PhysX:
			m_PhysicsAPI->Shutdown();
			break;
		case PhysicsAPIType::None: break;
		}
	}
}