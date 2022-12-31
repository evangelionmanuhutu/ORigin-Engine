﻿// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#include "pch.h"
#include "Scene.h"
#include "ScriptableEntity.h"
#include "Component\Component.h"

#include "Origin\Scripting\ScriptEngine.h"

#include "Origin\Renderer\Renderer2D.h"
#include "Origin\Scene\Skybox.h"

#include "Origin\Renderer\Renderer.h"
#include "Entity.h"

// Box2D
#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_circle_shape.h"

#include <glm\glm.hpp>
#include <unordered_map>

namespace Origin {

	static b2BodyType Box2DBodyType(Rigidbody2DComponent::BodyType type)
	{
		switch (type)
		{
			case Origin::Rigidbody2DComponent::BodyType::Static: return b2BodyType::b2_staticBody;
			case Origin::Rigidbody2DComponent::BodyType::Dynamic: return b2BodyType::b2_dynamicBody;
			case Origin::Rigidbody2DComponent::BodyType::Kinematic: return b2BodyType::b2_kinematicBody;
		}

		OGN_ASSERT(false, "Unkown Body Type");
		return b2_staticBody;
	}

	Scene::Scene()
	{
	}
	Scene::~Scene()
	{
	}

	template<typename... Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		([&]()
			{
				auto view = src.view<Component>();
		for (auto srcEntity : view)
		{
			entt::entity dstEntity = enttMap.at(src.get<IDComponent>(srcEntity).ID);

			auto& srcComponent = src.get<Component>(srcEntity);
			dst.emplace_or_replace<Component>(dstEntity, srcComponent);
		}
			}(), ...);
	}

	template<typename... Component>
	static void CopyComponent(ComponentGroup<Component...>, entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		CopyComponent<Component...>(dst, src, enttMap);
	}

	template<typename... Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		([&]()
			{
				if (src.HasComponent<Component>())
				dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
			}(), ...);
	}

	template<typename... Component>
	static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
	{
		CopyComponentIfExists<Component...>(dst, src);
	}

	std::shared_ptr<Scene> Scene::Copy(std::shared_ptr<Scene> other)
	{
		std::shared_ptr<Scene> newScene = std::make_shared<Scene>();

		newScene->m_ViewportWidth = other->m_ViewportWidth;
		newScene->m_ViewportHeight = other->m_ViewportHeight;

		auto& srcSceneRegistry = other->m_Registry;
		auto& dstSceneRegistry = newScene->m_Registry;
		std::unordered_map<UUID, entt::entity> enttMap;
		Entity newEntity = Entity();

		// create entities in new scene
		auto& idView = srcSceneRegistry.view<IDComponent>();
		for (auto e : idView)
		{
			UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;

			const auto& name = srcSceneRegistry.get<TagComponent>(e).Tag;
			newEntity = newScene->CreateEntityWithUUID(uuid, name);

			enttMap[uuid] = (entt::entity)newEntity;
		}

		// Copy components (except IDComponent and TagComponent) into new scene (destination)
		CopyComponent(AllComponents{}, dstSceneRegistry, srcSceneRegistry, enttMap);

		return newScene;
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateEntityWithUUID(UUID(), name);
	}

	Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;
		return entity;
	}

	Entity Scene::CreateSpriteEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(UUID());
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<SpriteRendererComponent>();

		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		return entity;
	}

	Entity Scene::CreateCamera(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };

		entity.AddComponent<IDComponent>();
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<CameraComponent>();

		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Camera" : name;

		auto& translation = entity.GetComponent<TransformComponent>().Translation;
		translation.z = 8.0f;

		return entity;
	}

	Entity Scene::CreateCircle(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };

		entity.AddComponent<IDComponent>();
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<CircleRendererComponent>();

		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Circle" : name;

		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	void Scene::OnUpdateRuntime(Timestep time)
	{
		// Update Scripts
		{
			auto& view = m_Registry.view<ScriptComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				ScriptEngine::OnUpdateEntity(entity, time);
			}

			m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
			{
				if (!nsc.Instance)
				{
					nsc.Instance = nsc.InstantiateScript();
					nsc.Instance->m_Entity = Entity{ entity, this };
					nsc.Instance->OnCreate();
				}
				nsc.Instance->OnUpdate(time);
			});
		}

		// Physics
		{
			const int32_t velocityIterations = 6;
			const int32_t positionIterations = 2;
			m_Box2DWorld->Step(time, velocityIterations, positionIterations);

			// Retrieve transform from Box2D
			auto view = m_Registry.view<Rigidbody2DComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				auto& transform = entity.GetComponent<TransformComponent>();
				auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

				b2Body* body = (b2Body*)rb2d.RuntimeBody;

				const auto& position = body->GetPosition();

				transform.Translation.x = position.x;
				transform.Translation.y = position.y;
				transform.Rotation.z = body->GetAngle();
			}
		}

		Camera* mainCamera = nullptr;
		glm::mat4 cameraTransform;

		auto& view = m_Registry.view<CameraComponent, TransformComponent>();
		for (auto& entity : view)
		{
			auto& [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

			if (camera.Primary)
			{
				mainCamera = &camera.Camera;
				cameraTransform = transform.GetTransform();
				break;
			}
		}

		if (mainCamera)
		{
			Renderer2D::BeginScene(*mainCamera, cameraTransform);

			// Sprites
			{
				auto& group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
				for (auto& entity : group)
				{
					auto& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
					Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
				}
			}

			// Circles
			{
				auto& view = m_Registry.view<TransformComponent, CircleRendererComponent>();
				for (auto& entity : view)
				{
					auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);
					Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade, (int)entity);
				}
			}
			Renderer2D::EndScene();
		}
	}

	void Scene::OnUpdateEditor(Timestep time, EditorCamera& camera)
	{

		m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
			{
				if (!nsc.Instance)
				{
					nsc.Instance = nsc.InstantiateScript();
					nsc.Instance->m_Entity = Entity{ entity, this };
					nsc.Instance->OnCreate();
				}
				nsc.Instance->OnUpdate(time);
			});

		//Render
		Render2DScene(camera);
	}

	void Scene::OnUpdateSimulation(Timestep time, EditorCamera& camera)
	{
		// Update Scripts
		{
			auto& view = m_Registry.view<ScriptComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				ScriptEngine::OnUpdateEntity(entity, time);
			}

			m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
			{
				if (!nsc.Instance)
				{
					nsc.Instance = nsc.InstantiateScript();
					nsc.Instance->m_Entity = Entity{ entity, this };
					nsc.Instance->OnCreate();
				}
				nsc.Instance->OnUpdate(time);
			});
		}

		// Physics
		{
			const int32_t velocityIterations = 6;
			const int32_t positionIterations = 2;
			m_Box2DWorld->Step(time, velocityIterations, positionIterations);

			// Retrieve transform from Box2D
			auto view = m_Registry.view<Rigidbody2DComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				auto& transform = entity.GetComponent<TransformComponent>();
				auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

				b2Body* body = (b2Body*)rb2d.RuntimeBody;

				const auto& position = body->GetPosition();

				transform.Translation.x = position.x;
				transform.Translation.y = position.y;
				transform.Rotation.z = body->GetAngle();
			}
		}

		// Render
		Render2DScene(camera);
	}

	void Scene::OnSimulationStart()
	{
		OnPhysics2DStart();

		// Scripting
		ScriptEngine::OnRuntimeStart(this);

		// instantiate all script entities
		auto view = m_Registry.view<ScriptComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };
			ScriptEngine::OnCreateEntity(entity);
		}
	}

	void Scene::OnSimulationStop()
	{
		OnPhysics2DStop();

		// Scripting
		ScriptEngine::OnRuntimeStop();
	}

	void Scene::Render2DScene(EditorCamera& camera)
	{
		Renderer2D::BeginScene(camera);

		// Sprites
		{
			auto& group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto& entity : group)
			{
				auto& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
				Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);
			}
		}

		// Circles
		{
			auto& view = m_Registry.view<TransformComponent, CircleRendererComponent>();
			for (auto& entity : view)
			{
				auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);
				Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade, (int)entity);
			}
		}
		DrawGrid(m_GridSize, m_GridColor);
		Renderer2D::EndScene();
	}

	void Scene::OnRuntimeStart()
	{
		OnPhysics2DStart();

		// Scripting
		ScriptEngine::OnRuntimeStart(this);

		// instantiate all script entities
		auto view = m_Registry.view<ScriptComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };
			ScriptEngine::OnCreateEntity(entity);
		}
	}

	void Scene::OnRuntimeStop()
	{
		OnPhysics2DStop();

		// Scripting
		ScriptEngine::OnRuntimeStop();
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera.SetViewportSize(width, height);
		}
	}

	static uint32_t duplicateEntity = 0;
	void Scene::DuplicateEntity(Entity entity)
	{
		std::string name = entity.GetName();
		Entity newEntity = CreateEntity(name);

		CopyComponentIfExists(AllComponents{}, newEntity, entity);

		newEntity.GetComponent<TransformComponent>().Translation.x += 0.2f;
		newEntity.GetComponent<TransformComponent>().Translation.y += 0.2f;
	}

	void Scene::DrawGrid(int size, glm::vec4 color)
	{
		// 3D x and z axis
		Renderer2D::DrawLine(glm::vec3(-size, -1.0f, 0.0f), glm::vec3(size, -1.0f, 0.0), glm::vec4(1, 0, 0, 1));
		Renderer2D::DrawLine(glm::vec3(0.0f, -1.0f, -size), glm::vec3(0.0f, -1.0f, size), glm::vec4(0, 1, 0, 1));

		for (int i = 1; i < size; i++)
		{
			Renderer2D::DrawLine(glm::vec3(0.0f + i, -1.0f, -size), glm::vec3(0.0f + i, -1.0f, size), color);
			Renderer2D::DrawLine(glm::vec3(0.0f - i, -1.0f, -size), glm::vec3(0.0f - i, -1.0f, size), color);
			Renderer2D::DrawLine(glm::vec3(-size, -1.0f, 0.0f - i), glm::vec3(size, -1.0f, 0.0f - i), color);
			Renderer2D::DrawLine(glm::vec3(-size, -1.0f, 0.0f + i), glm::vec3(size, -1.0f, 0.0f + i), color);
		}
	}

	void Scene::SetGrid(int size, glm::vec4 color)
	{
		m_GridSize = size;
		m_GridColor = color;
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			const auto& camera = view.get<CameraComponent>(entity);
			if (camera.Primary)
				return Entity{ entity, this };
		}
		return {};
	}

	void Scene::OnPhysics2DStart()
	{
		m_Box2DWorld = new b2World({ 0.0f, -9.8f });

		auto view = m_Registry.view<Rigidbody2DComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

			b2BodyDef bodyDef;
			bodyDef.type = Box2DBodyType(rb2d.Type);
			bodyDef.position.Set(transform.Translation.x, transform.Translation.y);
			bodyDef.angle = transform.Rotation.z;

			b2Body* body = m_Box2DWorld->CreateBody(&bodyDef);
			body->SetFixedRotation(rb2d.FixedRotation);
			rb2d.RuntimeBody = body;

			if (entity.HasComponent<BoxCollider2DComponent>())
			{
				auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

				b2PolygonShape boxShape;
				boxShape.SetAsBox(bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y,
					b2Vec2(bc2d.Offset.x, bc2d.Offset.y), 0.0f);

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &boxShape;
				fixtureDef.density = bc2d.Density;
				fixtureDef.friction = bc2d.Friction;
				fixtureDef.restitution = bc2d.Restitution;
				fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;
				body->CreateFixture(&fixtureDef);
			}

			if (entity.HasComponent<CircleCollider2DComponent>())
			{
				auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

				b2CircleShape circleShape;
				circleShape.m_p.Set(cc2d.Offset.x, cc2d.Offset.y);
				circleShape.m_radius = transform.Scale.x * cc2d.Radius;

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &circleShape;
				fixtureDef.density = cc2d.Density;
				fixtureDef.friction = cc2d.Friction;
				fixtureDef.restitution = cc2d.Restitution;
				fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;
				body->CreateFixture(&fixtureDef);
			}
		}
	}

	void Scene::OnPhysics2DStop()
	{
		delete m_Box2DWorld;
		m_Box2DWorld = nullptr;
	}

	template<typename T> void Scene::OnComponentAdded(Entity entity, T& component) { }
	template<> void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component) { }
	template<> void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component) {}
	template<> void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)	{
		if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
			component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}
	template<> void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component) {}
	template<> void Scene::OnComponentAdded<CircleRendererComponent>(Entity entity, CircleRendererComponent& component) {}
	template<> void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component) {}
	template<> void Scene::OnComponentAdded<ScriptComponent>(Entity entity, ScriptComponent& component) {}
	template<> void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component) {}
	template<> void Scene::OnComponentAdded<Rigidbody2DComponent>(Entity entity, Rigidbody2DComponent& component) {}
	template<> void Scene::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component) {}
	template<> void Scene::OnComponentAdded<CircleCollider2DComponent>(Entity entity, CircleCollider2DComponent& component) {}
}