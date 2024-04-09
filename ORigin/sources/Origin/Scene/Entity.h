// Copyright (c) Evangelion Manuhutu | ORigin Engine

#pragma once
#include "Components.h"
#include "Scene.h"
#include "Origin\Physics\RigidbodyComponent.h"
#include "Origin\Physics\ColliderComponent.h"
#include "entt\entt.hpp"
#include "box2d\b2_contact.h"

namespace origin {

	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene);
		Entity(const Entity& other) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			OGN_CORE_ASSERT(!HasComponent<T>(), "[Entity::AddComponent] already has component!");
			T& component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
			m_Scene->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T, typename... Args>
		T& AddOrReplaceComponent(Args&&... args)
		{
			T& component = m_Scene->m_Registry.emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
			m_Scene->OnComponentAdded<T>(*this, component);
			return component;
		}

		template<typename T>
		T& GetComponent()
		{
			OGN_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Scene->m_Registry.has<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			OGN_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}

		UUID GetUUID() { return GetComponent<IDComponent>().ID; }
		std::string& GetTag() { return GetComponent<TagComponent>().Tag; }

		bool IsContactWith(const std::string& objName)
		{
			if (!HasComponent<Rigidbody2DComponent>())
				return false;

			auto& rb2d = GetComponent<Rigidbody2DComponent>();
			return rb2d.ContactWith == objName;
		}

		std::string& GetContactTag()
		{
			std::string objName;

			if (!HasComponent<Rigidbody2DComponent>())
				objName = "";

			auto& rb2d = GetComponent<Rigidbody2DComponent>();
			objName = rb2d.ContactWith;

			return objName;
		}

		bool HasParent() { return GetComponent<IDComponent>().Parent != 0; }
		bool HasChildren() { return GetComponent<IDComponent>().Children.size() > 0; }

		operator bool() const { return m_EntityHandle != entt::null && m_Scene != nullptr; }
		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return static_cast<uint32_t>(m_EntityHandle); }
		operator uintptr_t() const { return static_cast<uintptr_t>(m_EntityHandle); }
		bool operator==(const Entity& other) const { return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene; }
		bool operator!=(const Entity& other) const { return !(*this == other); }
		
		Scene* GetScene() const { return m_Scene; }

	private:
		entt::entity m_EntityHandle{entt::null};
		Scene* m_Scene = nullptr;
	};
}
