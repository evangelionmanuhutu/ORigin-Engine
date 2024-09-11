// Copyright (c) 2022-present Evangelion Manuhutu | ORigin Engine

#ifndef SCRIPTABLE_ENTITY_H
#define SCRIPTABLE_ENTITY_H

#include "Origin/Core/Time.h"
#include "Origin/Scene/Entity.h"

namespace origin
{
	class OGN_API ScriptableEntity
	{
	public:
		ScriptableEntity() = default;
		virtual ~ScriptableEntity() {}

		template<typename T>
		T& GetComponent() {

			if(m_Entity.HasComponent<T>())
				return m_Entity.GetComponent<T>();

			return m_Entity.AddComponent<T>();
		}

	protected:
		virtual void OnCreate() {}
		virtual void OnUpdate(Timestep time) {}
		virtual void OnDestroy() {}

	private:
		Entity m_Entity;

		friend class Scene;
		friend class ScriptLibrary;
	};
}

#endif