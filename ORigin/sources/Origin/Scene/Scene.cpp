﻿// Copyright (c) Evangelion Manuhutu | ORigin Engine

#include "pch.h"
#include "Entity.h"
#include "Scene.h"
#include "Origin/Audio/Audio.h"
#include "ScriptableEntity.h"
#include "Lighting.h"

#include "Origin/Animation/Animation.h"
#include "Origin/Scripting/ScriptEngine.h"
#include "Origin/Renderer/Renderer.h"
#include "Origin/Renderer/Renderer2D.h"
#include "Origin/Renderer/Renderer3D.h"
#include "origin/Physics/Contact2DListener.h"

// Box2D
#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_circle_shape.h"

#include <glm/glm.hpp>

#include "Origin\Asset\AssetManager.h"

namespace origin
{
	class BoxColliderComponent;
	class RigidbodyComponent;
	class SphereColliderComponent;

	static b2BodyType Box2DBodyType(Rigidbody2DComponent::BodyType type)
	{
		switch (type)
		{
		case Rigidbody2DComponent::BodyType::Static: return b2_staticBody;
		case Rigidbody2DComponent::BodyType::Dynamic: return b2_dynamicBody;
		case Rigidbody2DComponent::BodyType::Kinematic: return b2_kinematicBody;
		}

		OGN_ASSERT(false, "Unkown Body Type");
		return b2_staticBody;
	}

	std::shared_ptr<Skybox>Scene::m_Skybox;

	Scene::Scene()
	{
		if (!m_PhysicsScene)
			m_PhysicsScene = PhysicsScene::Create(this);

		m_CameraIcon = Renderer::GetGTexture("CameraIcon");
		m_LightingIcon = Renderer::GetGTexture("LightingIcon");
		m_AudioIcon = Renderer::GetGTexture("AudioIcon");

		if (!m_Skybox)
		{
			m_Skybox = Skybox::Create("Resources/Skybox/", ".jpg");
			m_Skybox->SetBlur(0.0f);
		}
	}

	Scene::~Scene()
	{
	}

	template <typename... Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src,
	                          const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		([&]()
		{
			auto view = src.view<Component>();
			for (auto srcEntity : view)
			{
				entt::entity dstEntity = enttMap.at(src.get<IDComponent>(srcEntity).ID);

				const auto& srcComponent = src.get<Component>(srcEntity);
				dst.emplace_or_replace<Component>(dstEntity, srcComponent);
			}
		}(), ...);
	}

	template <typename... Component>
	static void CopyComponent(ComponentGroup<Component...>, entt::registry& dst, entt::registry& src,
	                          const std::unordered_map<UUID, entt::entity> enttMap)
	{
		CopyComponent<Component...>(dst, src, enttMap);
	}

	template <typename... Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		([&]()
		{
			if (src.HasComponent<Component>())
				dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
		}(), ...);
	}

	template <typename... Component>
	static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
	{
		CopyComponentIfExists<Component...>(dst, src);
	}

	std::shared_ptr<Scene> Scene::Copy(std::shared_ptr<Scene> other)
	{
		auto newScene = std::make_shared<Scene>();

		newScene->m_ViewportWidth = other->m_ViewportWidth;
		newScene->m_ViewportHeight = other->m_ViewportHeight;

		entt::registry& srcSceneRegistry = other->m_Registry;
		entt::registry& dstSceneRegistry = newScene->m_Registry;
		std::unordered_map<UUID, entt::entity> enttMap;
		auto newEntity = Entity();

		// create entities in new scene
		auto idView = srcSceneRegistry.view<IDComponent>();
		for (auto e : idView)
		{
			UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;

			const auto name = srcSceneRegistry.get<TagComponent>(e).Tag;
			newEntity = newScene->CreateEntityWithUUID(uuid, name);

			enttMap[uuid] = static_cast<entt::entity>(newEntity);
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
		Entity entity = {m_Registry.create(), this};
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		m_EntityMap.insert(std::make_pair(uuid, entity));

		return entity;
	}

	Entity Scene::CreatePointlight()
	{
		Entity entity = {m_Registry.create(), this};
		entity.AddComponent<IDComponent>(UUID());
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<LightComponent>();

		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = "Point Light";

		UUID& uuid = entity.GetComponent<IDComponent>().ID;
		entity.GetComponent<TransformComponent>().Translation.y = 5.0f;

		entity.GetComponent<LightComponent>().Light = Lighting::Create(LightingType::Point);

		m_EntityMap.insert(std::make_pair(uuid, entity));

		return entity;
	}

	Entity Scene::CreateSpotLight()
	{
		Entity entity = {m_Registry.create(), this};
		entity.AddComponent<IDComponent>(UUID());
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<LightComponent>();

		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = "Spot Light";

		UUID& uuid = entity.GetComponent<IDComponent>().ID;
		entity.GetComponent<TransformComponent>().Translation.y = 3.0f;

		entity.GetComponent<LightComponent>().Light = Lighting::Create(LightingType::Spot);

		m_EntityMap.insert(std::make_pair(uuid, entity));

		return entity;
	}

	Entity Scene::CreateDirectionalLight()
	{
		Entity entity = {m_Registry.create(), this};
		entity.AddComponent<IDComponent>(UUID());
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<LightComponent>();

		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = "Directional Light";

		UUID& uuid = entity.GetComponent<IDComponent>().ID;
		entity.GetComponent<TransformComponent>().Rotation.x = glm::radians(-90.0f);

		entity.GetComponent<LightComponent>().Light = Lighting::Create(LightingType::Directional);

		FramebufferSpecification fbSpec;
		fbSpec.Width = 1080;
		fbSpec.Height = 1080;

		fbSpec.WrapMode = GL_CLAMP_TO_BORDER;
		fbSpec.ReadBuffer = false;

		fbSpec.Attachments = {FramebufferTextureFormat::DEPTH};
		//entity.GetComponent<LightComponent>().ShadowFb = Framebuffer::Create(fbSpec);

		m_EntityMap.insert(std::make_pair(uuid, entity));

		return entity;
	}

	Entity Scene::CreateMesh(const std::string& name)
	{
		Entity entity = {m_Registry.create(), this};
		entity.AddComponent<IDComponent>(UUID());
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<StaticMeshComponent>();

		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		UUID& uuid = entity.GetComponent<IDComponent>().ID;
		m_EntityMap.insert(std::make_pair(uuid, entity));

		return entity;
	}

	Entity Scene::CreateSpriteEntity(const std::string& name)
	{
		Entity entity = {m_Registry.create(), this};
		entity.AddComponent<IDComponent>(UUID());
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<SpriteRenderer2DComponent>();

		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		UUID& uuid = entity.GetComponent<IDComponent>().ID;
		m_EntityMap.insert(std::make_pair(uuid, entity));

		return entity;
	}

	Entity Scene::CreateCube(const std::string& name)
	{
		Entity entity = {m_Registry.create(), this};
		entity.AddComponent<IDComponent>(UUID());
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<SpriteRendererComponent>();

		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		UUID& uuid = entity.GetComponent<IDComponent>().ID;
		m_EntityMap.insert(std::make_pair(uuid, entity));

		return entity;
	}

	Entity Scene::CreateCamera(const std::string& name)
	{
		Entity entity = {m_Registry.create(), this};

		entity.AddComponent<IDComponent>();
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<CameraComponent>();

		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Camera" : name;

		auto& translation = entity.GetComponent<TransformComponent>().Translation;
		translation.z = 8.0f;

		UUID& uuid = entity.GetComponent<IDComponent>().ID;
		m_EntityMap.insert(std::make_pair(uuid, entity));

		return entity;
	}

	Entity Scene::CreateCircle(const std::string& name)
	{
		Entity entity = {m_Registry.create(), this};

		entity.AddComponent<IDComponent>();
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<CircleRendererComponent>();

		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Circle" : name;

		UUID& uuid = entity.GetComponent<IDComponent>().ID;
		m_EntityMap.insert(std::make_pair(uuid, entity));

		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_EntityMap.erase(entity.GetUUID());
		m_Registry.destroy(entity);
	}

	void Scene::OnUpdateRuntime(Timestep deltaTime)
	{
		if (!m_Paused || m_StepFrames-- > 0)
		{
			// Update Scripts
			auto& scriptView = m_Registry.view<ScriptComponent>();
			for (auto& e : scriptView)
			{
				Entity entity = {e, this};
				ScriptEngine::OnUpdateEntity(entity, deltaTime);
			}

			m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
			{
				if (!nsc.Instance)
				{
					nsc.Instance = nsc.InstantiateScript();
					nsc.Instance->m_Entity = Entity{entity, this};
					nsc.Instance->OnCreate();
				}
				nsc.Instance->OnUpdate(deltaTime);
			});

			// Physics
			m_PhysicsScene->Simulate(deltaTime);

			constexpr int32_t velocityIterations = 6;
			constexpr int32_t positionIterations = 2;

			m_Box2DWorld->Step(deltaTime, velocityIterations, positionIterations);

			// Retrieve transform from Box2D
			auto& view = m_Registry.view<Rigidbody2DComponent>();
			for (auto& e : view)
			{
				Entity entity = { e, this };
				auto& transform = entity.GetComponent<TransformComponent>();
				const auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

				const auto body = static_cast<b2Body*>(rb2d.RuntimeBody);

				if (body)
				{
					const auto& position = body->GetPosition();
					transform.Translation.x = position.x;
					transform.Translation.y = position.y;
					transform.Rotation.z = body->GetAngle();
				}
			}
		}

		// Rendering
		const auto& cameraView = m_Registry.view<CameraComponent, TransformComponent>();
		for (const auto entity : cameraView)
		{
			auto& [tc, camera] = cameraView.get<TransformComponent, CameraComponent>(entity);

			if (camera.Primary)
			{
				Renderer::BeginScene(camera.Camera, tc);
				RenderScene(camera.Camera, tc);
				Renderer::EndScene();
				break;
			}
		}

		// Particle Update
		m_Registry.view<ParticleComponent>().each([=](auto entity, auto& pc)
		{
			pc.Particle.OnUpdate(deltaTime);
		});

		// Animation
		const auto& animView = m_Registry.view<AnimationComponent>();
		for (const auto entity : animView)
		{
			auto& ac = animView.get<AnimationComponent>(entity);
			if (ac.State.HasAnimation())
				ac.State.GetAnimation().Update(deltaTime);
		}

		// Audio Update
		const auto& audioView = m_Registry.view<TransformComponent, AudioComponent>();
		for (const auto entity : audioView)
		{
			auto& [tc, ac] = audioView.get<TransformComponent, AudioComponent>(entity);
			const std::shared_ptr<Audio>& audio = AssetManager::GetAsset<Audio>(ac.Audio);
			if (audio)
			{
				audio->SetPitch(ac.Pitch);
				audio->SetDopplerLevel(ac.DopplerLevel);
				audio->SetMaxDistance(ac.MaxDistance);
				audio->SetMinDistance(ac.MinDistance);
				audio->SetLoop(ac.Looping);
				audio->SetGain(ac.Volume);
				if (ac.Spatial)
				{
					static glm::vec3 prevPos = tc.Translation;

					const glm::vec3& position = tc.Translation;
					const glm::vec3 delta = position - prevPos;

					prevPos = position;

					glm::vec3 velocity = (position - prevPos) / glm::vec3(deltaTime);
					audio->Set3DAttributes(tc.Translation, velocity);
				}
			}
		}

		const auto& audioListenerView = m_Registry.view<TransformComponent, AudioListenerComponent>();
		for (const auto entity : audioListenerView)
		{
			auto& [tc, al] = audioListenerView.get<TransformComponent, AudioListenerComponent>(entity);
			if (al.Enable)
				al.Listener.Set(tc.Translation, glm::vec3(1.0f), tc.GetForward(), tc.GetUp());
			AudioEngine::SetMute(!al.Enable);
		}

		AudioEngine::SystemUpdate();
	}

	void Scene::OnUpdateEditor(Timestep deltaTime, EditorCamera& editorCamera)
	{
		m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
		{
			if (!nsc.Instance)
			{
				nsc.Instance = nsc.InstantiateScript();
				nsc.Instance->m_Entity = Entity{entity, this};
				nsc.Instance->OnCreate();
			}
			nsc.Instance->OnUpdate(deltaTime);
		});

		//Render
		RenderScene(editorCamera);
		{
			m_Registry.view<ParticleComponent>().each([=](auto entity, auto& pc)
			{
				pc.Particle.OnUpdate(deltaTime);
			});
		}

		// Animation
		const auto& animView = m_Registry.view<AnimationComponent>();
		for (const auto entity : animView)
		{
			auto& ac = animView.get<AnimationComponent>(entity);
			if (ac.State.HasAnimation())
			{
				if (ac.State.Preview)
					ac.State.Update(deltaTime);
				else
					ac.State.GetAnimation().Reset();
			}
		}

		// Audio Update
		Renderer2D::BeginScene();
		const auto& audioView = m_Registry.view<TransformComponent, AudioComponent>();
		for (auto entity : audioView)
		{
			auto& [tc, ac] = audioView.get<TransformComponent, AudioComponent>(entity);

			const std::shared_ptr<Audio>& audio = AssetManager::GetAsset<Audio>(ac.Audio);
			if (audio)
			{
				audio->SetPitch(ac.Pitch);
				audio->SetDopplerLevel(ac.DopplerLevel);
				audio->SetMaxDistance(ac.MaxDistance);
				audio->SetMinDistance(ac.MinDistance);
				audio->SetLoop(ac.Looping);
				audio->SetGain(ac.Volume);
				if (ac.Spatial)
				{
					static glm::vec3 prevPos = tc.Translation;

					const glm::vec3& position = tc.Translation;
					const glm::vec3 delta = position - prevPos;

					prevPos = position;

					glm::vec3 velocity = (position - prevPos) / glm::vec3(deltaTime);
					audio->Set3DAttributes(tc.Translation, velocity);
				}
			}

			DrawIcon(editorCamera, static_cast<int>(entity), m_AudioIcon, tc, true);
		}
		Renderer2D::EndScene();

		editorCamera.UpdateAudioListener(deltaTime);
		AudioEngine::SetMute(false);
		AudioEngine::SystemUpdate();
	}

	void Scene::OnUpdateSimulation(Timestep deltaTime, EditorCamera& editorCamera)
	{
		if (!m_Paused || m_StepFrames-- > 0)
		{
			// Update Scripts
			{
				const auto& view = m_Registry.view<ScriptComponent>();
				for (const auto e : view)
				{
					Entity entity = {e, this};
					ScriptEngine::OnUpdateEntity(entity, deltaTime);
				}

				m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
				{
					if (!nsc.Instance)
					{
						nsc.Instance = nsc.InstantiateScript();
						nsc.Instance->m_Entity = Entity{entity, this};
						nsc.Instance->OnCreate();
					}
					nsc.Instance->OnUpdate(deltaTime);
				});
			}

			//Physics
			m_PhysicsScene->Simulate(deltaTime);


			constexpr int32_t velocityIterations = 6;
			constexpr int32_t positionIterations = 2;

			m_Box2DWorld->Step(deltaTime, velocityIterations, positionIterations);

			// Retrieve transform from Box2D
			const auto& view = m_Registry.view<Rigidbody2DComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				auto& transform = entity.GetComponent<TransformComponent>();
				const auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

				const auto& body = static_cast<b2Body*>(rb2d.RuntimeBody);
				const auto& position = body->GetPosition();
				transform.Translation.x = position.x;
				transform.Translation.y = position.y;
				transform.Rotation.z = body->GetAngle();
			}
		}

		// Render
		RenderScene(editorCamera);

		{
			m_Registry.view<ParticleComponent>().each([=](auto entity, auto& pc)
			{
				pc.Particle.OnUpdate(deltaTime);
			});
		}

		// Animation
		const auto& animView = m_Registry.view<AnimationComponent>();
		for (const auto entity : animView)
		{
			auto& ac = animView.get<AnimationComponent>(entity);
			if (ac.State.HasAnimation())
				ac.State.GetAnimation().Update(deltaTime);
		}

		// Audio Update
		Renderer2D::BeginScene();
		AudioEngine::SetMute(false);
		const auto& audioView = m_Registry.view<TransformComponent, AudioComponent>();
		for (auto entity : audioView)
		{
			auto& [tc, ac] = audioView.get<TransformComponent, AudioComponent>(entity);
			const std::shared_ptr<Audio>& audio = AssetManager::GetAsset<Audio>(ac.Audio);

			if (audio)
			{
				audio->SetPitch(ac.Pitch);
				audio->SetDopplerLevel(ac.DopplerLevel);
				audio->SetMaxDistance(ac.MaxDistance);
				audio->SetMinDistance(ac.MinDistance);
				audio->SetLoop(ac.Looping);
				audio->SetGain(ac.Volume);

				if (ac.Spatial)
				{
					static glm::vec3 prevPos = tc.Translation;

					const glm::vec3& position = tc.Translation;
					const glm::vec3 delta = position - prevPos;

					prevPos = position;

					glm::vec3 velocity = (position - prevPos) / glm::vec3(deltaTime);
					audio->Set3DAttributes(tc.Translation, velocity);
				}
			}

			DrawIcon(editorCamera, static_cast<int>(entity), m_AudioIcon, tc, true);
		}
		Renderer2D::EndScene();

		bool isMainCameraListening = false;
		const auto& audioListenerView = m_Registry.view<TransformComponent, AudioListenerComponent>();
		for (const auto entity : audioListenerView)
		{
			auto& [tc, al] = audioListenerView.get<TransformComponent, AudioListenerComponent>(entity);
			if (al.Enable)
			{
				static glm::vec3 prevPos = tc.Translation;
				const glm::vec3& position = tc.Translation;
				const glm::vec3 delta = position - prevPos;
				prevPos = position;
				glm::vec3 velocity = (position - prevPos) / glm::vec3(deltaTime);

				al.Listener.Set(tc.Translation, velocity, tc.GetForward(), tc.GetUp());
			}

			isMainCameraListening = al.Enable;
		}

		if (!isMainCameraListening)
			editorCamera.UpdateAudioListener(deltaTime);

		AudioEngine::SystemUpdate();
	}

	void Scene::OnSimulationStart()
	{
		m_PhysicsScene->OnSimulationStart();
		OnPhysics2DStart();

		// Scripting
		ScriptEngine::OnRuntimeStart(this);
		const auto& scriptView = m_Registry.view<ScriptComponent>();
		for (const auto e : scriptView)
		{
			Entity entity = {e, this};
			ScriptEngine::OnCreateEntity(entity);
		}

		// Audio
		const auto& audioView = m_Registry.view<AudioComponent>();
		for (auto& e : audioView)
		{
			auto& ac = audioView.get<AudioComponent>(e);
			const std::shared_ptr<Audio>& audio = AssetManager::GetAsset<Audio>(ac.Audio);
			if (audio)
			{
				audio->Play();
			}
				
		}
	}

	void Scene::OnSimulationStop()
	{
		OnPhysics2DStop();
		m_PhysicsScene->OnSimulationStop();
		ScriptEngine::OnRuntimeStop();

		// Audio
		auto view = m_Registry.view<AudioComponent>();
		for (auto& e : view)
		{
			auto& ac = view.get<AudioComponent>(e);
			const std::shared_ptr<Audio>& audio = AssetManager::GetAsset<Audio>(ac.Audio);
			if (audio)
			{
				audio->Stop();
			}
		}
	}

	void Scene::RenderScene(const Camera& camera, const TransformComponent& cameraTransform)
	{
		m_Skybox->Draw(camera);

		const auto& lightView = m_Registry.view<TransformComponent, LightComponent>();

		// Particle
		{
			auto& view = m_Registry.view<TransformComponent, ParticleComponent>();
			for (auto entity : view)
			{
				auto& [tc, pc] = view.get<TransformComponent, ParticleComponent>(entity);

				for (int i = 0; i < 5; i++)
				{
					pc.Particle.Emit(
						pc,
						glm::vec3(tc.Translation.x, tc.Translation.y, tc.Translation.z + pc.ZAxis),
						tc.Scale, tc.Rotation, static_cast<int>(entity)
					);
				}
				pc.Particle.OnRender();
			}
		}

		// Sprites
		{
			const auto& view = m_Registry.view<TransformComponent, SpriteRenderer2DComponent>();
			for (const entt::entity& entity : view)
			{
				auto& [transform, sprite] = view.get<TransformComponent, SpriteRenderer2DComponent>(entity);
				Renderer2D::DrawSprite(transform.GetTransform(), sprite, static_cast<int>(entity));
			}

			const auto& animView = m_Registry.view<SpriteRenderer2DComponent, AnimationComponent>();
			for (auto& entity : animView)
			{
				auto& [sprite, anim] = animView.get<SpriteRenderer2DComponent, AnimationComponent>(entity);
				if (anim.State.HasAnimation() == false)
					continue;

				std::shared_ptr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(sprite.Texture);

				if (anim.State.GetAnimation().HasFrame())
				{
					texture = anim.State.GetAnimation().GetCurrentSprite();
				}
				else
				{
					if (texture)
					{
						texture->Delete();
						texture.reset();
					}
				}
			}
		}

		// Circles
		{
			auto& view = m_Registry.view<TransformComponent, CircleRendererComponent>();
			for (auto& entity : view)
			{
				auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);
				Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade,
				                       static_cast<int>(entity));
			}
		}

		// Text
		const auto& textView = m_Registry.view<TransformComponent, TextComponent>();
		for (const auto entity : textView)
		{
			auto [transform, text] = textView.get<TransformComponent, TextComponent>(entity);
			Renderer2D::DrawString(text.TextString, transform.GetTransform(), text, static_cast<int>(entity));
		}

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		// 3D Scene
		const auto& spriteView = m_Registry.view<TransformComponent, SpriteRendererComponent>();
		for (const auto entity : spriteView)
		{
			auto& [transform, sprite] = spriteView.get<TransformComponent, SpriteRendererComponent>(entity);
			Renderer3D::DrawCube(transform.GetTransform(), sprite, static_cast<int>(entity));
		}

		// Mesh
		{
			const auto& meshView = m_Registry.view<TransformComponent, StaticMeshComponent>();
			for (auto& entity : meshView)
			{
				auto& [tc, mesh] = meshView.get<TransformComponent, StaticMeshComponent>(entity);

				Lighting::PointLightCount = 0;
				Lighting::SpotLightCount = 0;

				if (mesh.Model)
				{
					mesh.Material->EnableShader();
					mesh.Material->SetFloat("material.Bias", mesh.Material->Bias);

					for (auto& light : lightView)
					{
						auto& [tc, lc] = lightView.get<TransformComponent, LightComponent>(light);
						lc.Light->OnUpdate(tc, mesh.Material);
					}

					mesh.Model->Draw(tc.GetTransform(), camera, cameraTransform.GetTransform(), static_cast<int>(entity));
					mesh.Material->DisableShader();

					if (mesh.Material->m_PointLightCount != Lighting::PointLightCount ||
						mesh.Material->m_SpotLightCount != Lighting::SpotLightCount)
						mesh.Material->RefreshShader();

					mesh.Material->m_PointLightCount = Lighting::PointLightCount;
					mesh.Material->m_SpotLightCount = Lighting::SpotLightCount;
				}
			}
		}
		

		glDisable(GL_CULL_FACE);
	}

	void Scene::RenderScene(const EditorCamera& camera)
	{

		Renderer::BeginScene(camera);

		m_Skybox->Draw(camera);

		const auto& lightView = m_Registry.view<TransformComponent, LightComponent>();
		for (auto& entity : lightView)
		{
			auto& [tc, lc] = lightView.get<TransformComponent, LightComponent>(entity);
			DrawIcon(camera, static_cast<int>(entity), m_LightingIcon, tc, true);
		}

		// Particle
		{
			auto& view = m_Registry.view<TransformComponent, ParticleComponent>();
			for (auto entity : view)
			{
				auto& [tc, pc] = view.get<TransformComponent, ParticleComponent>(entity);

				for (int i = 0; i < 5; i++)
				{
					pc.Particle.Emit(pc,
					                 glm::vec3(tc.Translation.x, tc.Translation.y, tc.Translation.z + pc.ZAxis),
					                 tc.Scale, pc.Rotation, static_cast<int>(entity)
					);
				}

				pc.Particle.OnRender();
			}
		}

		// Sprites
		{
			auto& view = m_Registry.view<TransformComponent, SpriteRenderer2DComponent>();
			std::vector<entt::entity> spriteEntities(view.begin(), view.end());

			std::sort(spriteEntities.begin(), spriteEntities.end(),
			          [=](const entt::entity& a, const entt::entity& b)
			          {
				          const auto& objA = m_Registry.get<TransformComponent>(a);
				          const auto& objB = m_Registry.get<TransformComponent>(b);
				          return length(camera.GetPosition() - objA.Translation) > length(
					          camera.GetPosition() - objB.Translation);
			          });

			for (const entt::entity& entity : spriteEntities)
			{
				auto& [transform, sprite] = view.get<TransformComponent, SpriteRenderer2DComponent>(entity);

				for (auto& light : lightView)
				{
					auto& [tc, lc] = lightView.get<TransformComponent, LightComponent>(light);
					lc.Light->OnUpdate(tc);
				}

				Renderer2D::DrawSprite(transform.GetTransform(), sprite, static_cast<int>(entity));
			}

			const auto& animView = m_Registry.view<SpriteRenderer2DComponent, AnimationComponent>();
			for (auto& entity : animView)
			{
				auto& [sprite, anim] = animView.get<SpriteRenderer2DComponent, AnimationComponent>(entity);
				if (anim.State.HasAnimation() == false)
					continue;

				std::shared_ptr<Texture2D> texture = AssetManager::GetAsset<Texture2D>(sprite.Texture);

				if (anim.State.GetAnimation().HasFrame())
				{
					texture = anim.State.GetAnimation().GetCurrentSprite();
				}
				else
				{
					if (texture)
					{
						texture->Delete();
						texture.reset();
					}
				}
			}
		}

		// Circles
		{
			const auto& view = m_Registry.view<TransformComponent, CircleRendererComponent>();
			for (auto& entity : view)
			{
				auto& [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);
				for (auto& light : lightView)
				{
					auto& [tc, lc] = lightView.get<TransformComponent, LightComponent>(light);
					lc.Light->OnUpdate(tc);
				}
				Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade,
				                       static_cast<int>(entity));
			}
		}

		// Text
		{
			const auto& view = m_Registry.view<TransformComponent, TextComponent>();
			for (auto entity : view)
			{
				auto [transform, text] = view.get<TransformComponent, TextComponent>(entity);
				for (auto& light : lightView)
				{
					auto& [tc, lc] = lightView.get<TransformComponent, LightComponent>(light);
					lc.Light->OnUpdate(tc);
				}
				Renderer2D::DrawString(text.TextString, transform.GetTransform(), text, static_cast<int>(entity));
			}
		}

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		// 3D Scene
		// Cube
		const auto& cubeView = m_Registry.view<TransformComponent, SpriteRendererComponent>();
		std::vector<entt::entity> cubeEntities(cubeView.begin(), cubeView.end());
		std::sort(cubeEntities.begin(), cubeEntities.end(),
		          [=](const entt::entity& a, const entt::entity& b)
		          {
			          const auto& objA = m_Registry.get<TransformComponent>(a);
			          const auto& objB = m_Registry.get<TransformComponent>(b);
			          return length(camera.GetPosition() - objA.Translation) > length(
				          camera.GetPosition() - objB.Translation);
		          });

		for (const entt::entity cube : cubeEntities)
		{
			auto& [transform, sprite] = cubeView.get<TransformComponent, SpriteRendererComponent>(cube);
			Renderer3D::DrawCube(transform.GetTransform(), sprite, static_cast<int>(cube));
		}

		// Camera
		const auto& cameraView = m_Registry.view<TransformComponent, CameraComponent>();
		for (auto& entity : cameraView)
		{
			auto& [tc, cc] = cameraView.get<TransformComponent, CameraComponent>(entity);
			DrawIcon(camera, static_cast<int>(entity), m_CameraIcon, tc, true);
		}

		

		// Mesh
		const auto& meshView = m_Registry.view<TransformComponent, StaticMeshComponent>();
		for (auto& entity : meshView)
		{
			auto& [tc, mesh] = meshView.get<TransformComponent, StaticMeshComponent>(entity);
			
			Lighting::PointLightCount = 0;
			Lighting::SpotLightCount = 0;

			if (mesh.Model)
			{
				mesh.Material->EnableShader();
				mesh.Material->SetFloat("material.Bias", mesh.Material->Bias);

				for (auto& light : lightView)
				{
					auto& [tc, lc] = lightView.get<TransformComponent, LightComponent>(light);
					lc.Light->OnUpdate(tc, mesh.Material);
				}

				mesh.Model->Draw(tc.GetTransform(), camera, static_cast<int>(entity));
				mesh.Material->DisableShader();

				if (mesh.Material->m_PointLightCount != Lighting::PointLightCount || 
					mesh.Material->m_SpotLightCount != Lighting::SpotLightCount)
					mesh.Material->RefreshShader();

				mesh.Material->m_PointLightCount = Lighting::PointLightCount;
				mesh.Material->m_SpotLightCount = Lighting::SpotLightCount;
			}
		}
		glDisable(GL_CULL_FACE);

		//DrawGrid(m_GridSize, m_GridColor);
		Renderer::EndScene();
	}

	void Scene::OnShadowRender()
	{
		// ==============================
		// Directional Light Shadow
		// ==============================
		Renderer::GetGShader("DirLightDepthMap")->Enable();

		const auto& dirLight = m_Registry.view<TransformComponent, LightComponent>();
		for (auto& light : dirLight)
		{
			auto& [tc, lc] = dirLight.get<TransformComponent, LightComponent>(light);
			lc.Light->SetupShadow(tc);

			// Render
			const auto& meshView = m_Registry.view<TransformComponent, StaticMeshComponent>();
			for (auto& entity : meshView)
			{
				auto& [tc, mesh] = meshView.get<TransformComponent, StaticMeshComponent>(entity);

				if (mesh.Model)
				{
					// Set Depth shader
					Renderer::GetGShader("DirLightDepthMap")->SetMatrix("uModel", tc.GetTransform());
					mesh.Model->Draw();
				}
			}

			lc.Light->GetShadow()->GetFramebuffer()->Unbind();
		}

		Renderer::GetGShader("DirLightDepthMap")->Disable();

		glDisable(GL_CULL_FACE);
	}

	void Scene::OnRuntimeStart()
	{
		m_PhysicsScene->OnSimulationStart();
		OnPhysics2DStart();

		// Scripting
		ScriptEngine::OnRuntimeStart(this);
		auto scriptView = m_Registry.view<ScriptComponent>();
		for (auto e : scriptView)
		{
			Entity entity = {e, this};
			ScriptEngine::OnCreateEntity(entity);
		}

		// Audio
		auto audioView = m_Registry.view<AudioComponent>();
		for (auto& e : audioView)
		{
			auto& ac = audioView.get<AudioComponent>(e);
			const std::shared_ptr<Audio>& audio = AssetManager::GetAsset<Audio>(ac.Audio);
			if (audio && ac.PlayAtStart)
			{
				audio->SetGain(ac.Volume);
				audio->Play();
			}
		}
	}

	void Scene::OnRuntimeStop()
	{
		OnPhysics2DStop();
		m_PhysicsScene->OnSimulationStop();

		ScriptEngine::OnRuntimeStop();

		// Audio
		const auto& audioView = m_Registry.view<AudioComponent>();
		for (auto& e : audioView)
		{
			auto& ac = audioView.get<AudioComponent>(e);
			const std::shared_ptr<Audio>& audio = AssetManager::GetAsset<Audio>(ac.Audio);
			if (audio)
				audio->Stop();
		}
	}

	void Scene::OnViewportResize(const uint32_t width, const uint32_t height)
	{
		if (m_ViewportHeight == height && m_ViewportWidth == width)
			return;

		m_ViewportWidth = width;
		m_ViewportHeight = height;

		const auto& view = m_Registry.view<CameraComponent>();
		for (auto& entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera.SetViewportSize(width, height);
		}
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		std::string name = entity.GetTag();
		Entity newEntity = CreateEntity(name);

		CopyComponentIfExists(AllComponents{}, newEntity, entity);

		return newEntity;
	}

	void Scene::DrawGrid(int size, glm::vec4 color)
	{
		// 3D XZ axis
		Renderer2D::DrawLine(glm::vec3(-size, -1.0f, 0.0f), glm::vec3(size, -1.0f, 0.0), glm::vec4(1, 0, 0, 1));
		Renderer2D::DrawLine(glm::vec3(0.0f, -1.0f, -size), glm::vec3(0.0f, -1.0f, size), glm::vec4(0, 0, 1, 1));

		// TODO: XY axis
		for (float i = 1.0f; i <= size; i++)
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

	Entity Scene::GetEntityWithUUID(UUID uuid)
	{
		if (m_EntityMap.find(uuid) != m_EntityMap.end())
			return {m_EntityMap.at(uuid), this};

		return {};
	}

	Entity Scene::FindEntityByName(std::string_view name)
	{
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			const TagComponent& tc = view.get<TagComponent>(entity);
			if (tc.Tag == name)
				return Entity{entity, this};
		}

		return {};
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		const auto& view = m_Registry.view<CameraComponent>();
		for (auto& entity : view)
		{
			const CameraComponent& camera = view.get<CameraComponent>(entity);
			if (camera.Primary)
				return Entity{entity, this};
		}
		return {};
	}

	void Scene::OnPhysics2DStart()
	{
		m_Running = true;
		m_Box2DWorld = new b2World({0.0f, -9.81f});

		m_Box2DContactListener = new Contact2DListener(this);
		m_Box2DWorld->SetContactListener(m_Box2DContactListener);

		auto view = m_Registry.view<Rigidbody2DComponent>();
		for (entt::entity e : view)
		{
			Entity entity = {e, this};
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

			b2BodyDef bodyDef;

			// set user data based on the entity
			bodyDef.userData.pointer = static_cast<uintptr_t>(e);

			bodyDef.type = Box2DBodyType(rb2d.Type);
			bodyDef.linearDamping = rb2d.LinearDamping;
			bodyDef.angularDamping = rb2d.AngularDamping;
			bodyDef.allowSleep = rb2d.AllowSleeping;
			bodyDef.awake = rb2d.Awake;
			bodyDef.bullet = rb2d.Bullet;
			bodyDef.enabled = rb2d.Enabled;

			// POSITION SETTINGS
			float xPos = transform.Translation.x;
			float yPos = transform.Translation.y;

			if (rb2d.FreezePositionX)
				xPos = bodyDef.position.x;
			if (rb2d.FreezePositionY)
				yPos = bodyDef.position.y;

			bodyDef.position.Set(xPos, yPos);

			// ROTATION SETTINGS
			bodyDef.angle = transform.Rotation.z;

			bodyDef.gravityScale = rb2d.GravityScale;

			b2Body* body = m_Box2DWorld->CreateBody(&bodyDef);
			body->SetFixedRotation(rb2d.FixedRotation);

			b2MassData massData;
			massData.center = b2Vec2(rb2d.MassCenter.x, rb2d.MassCenter.y);
			massData.I = rb2d.RotationalInertia;
			massData.mass = rb2d.Mass;
			body->SetMassData(&massData);

			rb2d.RuntimeBody = body;

			if (entity.HasComponent<BoxCollider2DComponent>())
			{
				auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

				b2PolygonShape boxShape;
				boxShape.SetAsBox(
					bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y,
				  b2Vec2(bc2d.Offset.x, bc2d.Offset.y), 0.0f
				);

				bc2d.RuntimeBoxShape = &boxShape;

				b2FixtureDef fixtureDef;
				fixtureDef.filter.groupIndex = bc2d.Group;
				fixtureDef.shape = &boxShape;
				fixtureDef.density = bc2d.Density;
				fixtureDef.friction = bc2d.Friction;
				fixtureDef.restitution = bc2d.Restitution;
				fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;

				bc2d.RuntimeFixture = &fixtureDef;

				body->CreateFixture(&fixtureDef);
			}

			if (entity.HasComponent<CircleCollider2DComponent>())
			{
				auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

				b2CircleShape circleShape;
				circleShape.m_p.Set(cc2d.Offset.x, cc2d.Offset.y);
				circleShape.m_radius = transform.Scale.x * cc2d.Radius;

				cc2d.RuntimeCircleShape = &circleShape;

				b2FixtureDef fixtureDef;
				fixtureDef.filter.groupIndex = cc2d.Group;
				fixtureDef.shape = &circleShape;
				fixtureDef.density = cc2d.Density;
				fixtureDef.friction = cc2d.Friction;
				fixtureDef.restitution = cc2d.Restitution;
				fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;

				cc2d.RuntimeFixture = &fixtureDef;

				body->CreateFixture(&fixtureDef);
			}
		}
	}

	void Scene::OnPhysics2DStop()
	{
		m_Running = false;

		delete m_Box2DContactListener;
		m_Box2DContactListener = nullptr;

		m_Box2DWorld->SetContactListener(nullptr);

		delete m_Box2DWorld;
		m_Box2DWorld = nullptr;
	}

	void Scene::DrawIcon(const EditorCamera& camera, int entity, const std::shared_ptr<Texture2D>& texture,
	                     const TransformComponent& tc, bool rotate)
	{
		const glm::mat4 transform = translate(glm::mat4(1.0f), tc.Translation)
			* glm::rotate(glm::mat4(1.0f), -camera.GetYaw(), glm::vec3(0, 1, 0))
			* glm::rotate(glm::mat4(1.0f), -camera.GetPitch(), glm::vec3(1, 0, 0))
			* scale(glm::mat4(1.0f), glm::vec3(1.0f));

		Renderer2D::DrawQuad(transform, texture, glm::vec2(1.0f), glm::vec4(1.0f), entity);
	}

	void Scene::Step(int frames)
	{
		m_StepFrames = frames;
	}

	template <typename T>
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
	}

#define OGN_REG_COMPONENT(components)\
template<>\
void Scene::OnComponentAdded<components>(Entity entity, components& component){}

	OGN_REG_COMPONENT(IDComponent)
	OGN_REG_COMPONENT(TransformComponent)
	OGN_REG_COMPONENT(AudioComponent)
	OGN_REG_COMPONENT(AudioListenerComponent)
	OGN_REG_COMPONENT(AnimationComponent)
	OGN_REG_COMPONENT(SpriteRendererComponent)
	OGN_REG_COMPONENT(SpriteRenderer2DComponent)
	OGN_REG_COMPONENT(LightComponent)
	OGN_REG_COMPONENT(StaticMeshComponent)
	OGN_REG_COMPONENT(TextComponent)
	OGN_REG_COMPONENT(CircleRendererComponent)
	OGN_REG_COMPONENT(TagComponent)
	OGN_REG_COMPONENT(NativeScriptComponent)
	OGN_REG_COMPONENT(ScriptComponent)
	OGN_REG_COMPONENT(Rigidbody2DComponent)
	OGN_REG_COMPONENT(BoxCollider2DComponent)
	OGN_REG_COMPONENT(CircleCollider2DComponent)
	OGN_REG_COMPONENT(ParticleComponent)
	OGN_REG_COMPONENT(RigidbodyComponent)
	OGN_REG_COMPONENT(BoxColliderComponent)
	OGN_REG_COMPONENT(SphereColliderComponent)

	template <>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
			component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}
}
