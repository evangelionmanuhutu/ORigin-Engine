﻿// Copyright (c) Evangelion Manuhutu | ORigin Engine

#include "pch.h"
#include "Scene.h"
#include "Entity.h"
#include "Lighting.h"
#include "EntityManager.h"
#include "ScriptableEntity.h"
#include "Origin/Audio/AudioEngine.h"
#include "Origin/Audio/AudioSource.h"
#include "Origin/Instrumetation/Instrumentor.h"
#include "Origin/Animation/Animation.h"
#include "origin/Physics/Contact2DListener.h"
#include "Origin/Physics/Physics2D.h"
#include "Origin/Renderer/Renderer.h"
#include "Origin/Renderer/Renderer2D.h"
#include "Origin/Renderer/Renderer3D.h"
#include "Origin/Scripting/ScriptEngine.h"
#include "Origin/Asset/AssetManager.h"
#include <glm/glm.hpp>

#pragma warning(disable : OGN_DISABLED_WARNINGS)

namespace origin
{
	class BoxColliderComponent;
	class RigidbodyComponent;
	class SphereColliderComponent;
	class CapsuleColliderComponent;

	Scene::Scene()
	{
		PROFILER_SCENE();

		if (!m_PhysicsScene)
			m_PhysicsScene = PhysicsScene::Create(this);

		m_Physics2D = new Physics2D(this);
	}

	Scene::~Scene()
	{
		PROFILER_SCENE();
		delete m_Physics2D;
	}

	std::shared_ptr<Scene> Scene::Copy(std::shared_ptr<Scene> other)
	{
		PROFILER_SCENE();

		auto newScene = std::make_shared<Scene>();

		newScene->m_ViewportWidth = other->m_ViewportWidth;
		newScene->m_ViewportHeight = other->m_ViewportHeight;

		entt::registry &srcSceneRegistry = other->m_Registry;
		entt::registry &dstSceneRegistry = newScene->m_Registry;
		std::vector<std::tuple<UUID, entt::entity>> enttStorage;
		auto newEntity = Entity();

		// create entities in new scene
		auto idView = srcSceneRegistry.view<IDComponent>();
		for (auto e : idView)
		{
			auto idc = srcSceneRegistry.get<IDComponent>(e);
			const auto name = srcSceneRegistry.get<TagComponent>(e).Tag;
			newEntity = EntityManager::CreateEntityWithUUID(idc.ID, name, newScene.get());
			auto &eIDC = newEntity.GetComponent<IDComponent>();
			eIDC.Parent = idc.Parent;
			eIDC.Children = idc.Children;

			enttStorage.push_back({ idc.ID, static_cast<entt::entity>(newEntity) });
		}

		EntityManager::CopyComponent(AllComponents{}, dstSceneRegistry, srcSceneRegistry, enttStorage);
		return newScene;
	}

	Entity Scene::GetEntityWithUUID(UUID uuid)
	{
		PROFILER_SCENE();

		for (auto e : m_EntityStorage)
		{
			if (std::get<0>(e) == uuid)
				return { std::get<1>(e), this };
		}

		return {};
	}

	Entity Scene::FindEntityByName(std::string_view name)
	{
		PROFILER_SCENE();

		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			const TagComponent& tc = view.get<TagComponent>(entity);
			if (tc.Tag == name)
				return { entity, this };
		}

		return {};
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		PROFILER_SCENE();

		const auto &view = m_Registry.view<CameraComponent>();
		for (auto &entity : view)
		{
			const CameraComponent &camera = view.get<CameraComponent>(entity);
			if (camera.Primary)
				return { entity, this };
		}
		return {};
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{
		PROFILER_SCENE();

		if (!m_Paused || m_StepFrames-- > 0)
		{
			// Update Scripts
			auto& scriptView = m_Registry.view<ScriptComponent>();
			for (auto e : scriptView)
			{
				Entity entity = {e, this};
				ScriptEngine::OnUpdateEntity(entity, (float)ts);
			}

			m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
			{
				if (nsc.Instance)
					nsc.Instance->OnUpdate(ts);
			});

			// Particle Update
			m_Registry.view<ParticleComponent>().each([=](auto entity, auto& pc)
				{
					pc.Particle.OnUpdate(ts);
				});


			// Animation
			const auto& animView = m_Registry.view<SpriteAnimationComponent>();
			for (const auto entity : animView)
			{
				auto& ac = animView.get<SpriteAnimationComponent>(entity);
				if (ac.State->IsCurrentAnimationExists())
				{
					ac.State->OnUpdateRuntime(ts);
				}
			}

			// Audio Update
			const auto& audioView = m_Registry.view<TransformComponent, AudioComponent>();
			for (auto entity : audioView)
			{
				auto& [tc, ac] = audioView.get<TransformComponent, AudioComponent>(entity);

				if (std::shared_ptr<AudioSource>& audio = AssetManager::GetAsset<AudioSource>(ac.Audio))
				{
					audio->SetVolume(ac.Volume);
					audio->SetPitch(ac.Pitch);
					audio->SetPaning(ac.Panning);
					audio->SetLoop(ac.Looping);
					audio->SetSpatial(ac.Spatializing);
					if (ac.Spatializing)
					{
						audio->SetMinMaxDistance(ac.MinDistance, ac.MaxDistance);
						audio->SetPosition(tc.Translation);
					}
				}
			}

			const auto& audioListenerView = m_Registry.view<TransformComponent, AudioListenerComponent>();
			for (auto entity : audioListenerView)
			{
				auto& [tc, al] = audioListenerView.get<TransformComponent, AudioListenerComponent>(entity);
			}
			m_PhysicsScene->Simulate(ts);
			m_Physics2D->Simulate(ts);
		}
		

		// Rendering
		auto& cameraView = m_Registry.view<CameraComponent, TransformComponent>();
		for (auto entity : cameraView)
		{
			auto& [tc, camera] = cameraView.get<TransformComponent, CameraComponent>(entity);

			if (camera.Primary)
			{
				RenderScene(camera.Camera, tc);
				UpdateEditorTransform();
				break;
			}
		}
	}

	void Scene::OnRuntimeStart()
	{
		PROFILER_SCENE();

		m_Running = true;

		ScriptEngine::SetSceneContext(this);
		auto scriptView = m_Registry.view<ScriptComponent>();
		for (auto e : scriptView)
		{
			Entity entity = { e, this };
			ScriptEngine::OnCreateEntity(entity);
		}

		m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto &nsc)
		{
			if(!nsc.Instance)
			{
				nsc.Instance = nsc.InstantiateScript();
				nsc.Instance->m_Entity = Entity { entity, this };	
			}
		});

		auto audioView = m_Registry.view<AudioComponent>();
		for (auto& e : audioView)
		{
			auto& ac = audioView.get<AudioComponent>(e);
			std::shared_ptr<AudioSource>& audio = AssetManager::GetAsset<AudioSource>(ac.Audio);
			if (ac.PlayAtStart)
				audio->Play();
		}

		m_PhysicsScene->OnSimulationStart();
		m_Physics2D->OnSimulationStart();
	}

	void Scene::OnRuntimeStop()
	{
		PROFILER_SCENE();

		m_Running = false;
		ScriptEngine::ClearSceneContext();
		m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
		{
			nsc.DestroyScript(&nsc);
		});

		const auto& audioView = m_Registry.view<AudioComponent>();

		for (auto& e : audioView)
		{
			auto& ac = audioView.get<AudioComponent>(e);
			if (const std::shared_ptr<AudioSource>& audio = AssetManager::GetAsset<AudioSource>(ac.Audio))
				audio->Stop();
		}

		m_PhysicsScene->OnSimulationStop();
		m_Physics2D->OnSimulationStop();
	}

	void Scene::OnEditorUpdate(Timestep ts, EditorCamera& editorCamera)
	{
		PROFILER_RENDERING();

		m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
		{
			nsc.Instance->OnUpdate(ts);
		});

		m_Registry.view<ParticleComponent>().each([=](auto entity, auto& pc)
		{
			pc.Particle.OnUpdate(ts);
		});

		// Animation
		const auto& animView = m_Registry.view<SpriteAnimationComponent>();
		for (const auto entity : animView)
		{
			auto& ac = animView.get<SpriteAnimationComponent>(entity);
			if(ac.State->IsCurrentAnimationExists())
				ac.State->OnUpdateEditor(ts);
		}

		// Audio Update
		Renderer2D::Begin(editorCamera);
		const auto& audioView = m_Registry.view<TransformComponent, AudioComponent>();
		for (auto entity : audioView)
		{
			auto& [tc, ac] = audioView.get<TransformComponent, AudioComponent>(entity);
			if (const std::shared_ptr<AudioSource> &audio = AssetManager::GetAsset<AudioSource>(ac.Audio))
			{
				audio->SetVolume(ac.Volume);
				audio->SetPitch(ac.Pitch);
				audio->SetPaning(ac.Panning);
				audio->SetLoop(ac.Looping);
				audio->SetSpatial(ac.Spatializing);
				if (ac.Spatializing)
				{
					audio->SetMinMaxDistance(ac.MinDistance, ac.MaxDistance);
					audio->SetPosition(tc.Translation);
				}
			}
		}
		Renderer2D::End();
		editorCamera.UpdateAudioListener(ts);

		RenderScene(editorCamera);
		UpdateEditorTransform();
	}

	void Scene::OnUpdateSimulation(Timestep ts, EditorCamera& editorCamera)
	{
		PROFILER_RENDERING();

		if (!m_Paused || m_StepFrames-- > 0)
		{
			// Update Scripts
			m_Registry.view<ScriptComponent>().each([=](auto entityID, auto& sc)
			{
				Entity entity{ entityID, this };
				ScriptEngine::OnUpdateEntity(entity, (float)ts);
			});

			m_Registry.view<NativeScriptComponent>().each([=](auto entityID, auto& nsc)
			{
				nsc.Instance->OnUpdate(ts);
			});

			m_Registry.view<ParticleComponent>().each([=](auto entity, auto& pc)
			{
				pc.Particle.OnUpdate(ts);
			});

			// Animation
			auto& animView = m_Registry.view<SpriteAnimationComponent>();
			for (auto e : animView)
			{
				auto ac = animView.get<SpriteAnimationComponent>(e);
				if (ac.State->IsCurrentAnimationExists())
					ac.State->OnUpdateRuntime(ts);
			}

			Renderer2D::Begin(editorCamera);
			const auto& audioView = m_Registry.view<TransformComponent, AudioComponent>();
			for (auto entity : audioView)
			{
				auto& [tc, ac] = audioView.get<TransformComponent, AudioComponent>(entity);
				if (std::shared_ptr<AudioSource> &audio = AssetManager::GetAsset<AudioSource>(ac.Audio))
				{
					audio->SetVolume(ac.Volume);
					audio->SetPitch(ac.Pitch);
					audio->SetPaning(ac.Panning);
					audio->SetLoop(ac.Looping);
					audio->SetSpatial(ac.Spatializing);
					if (ac.Spatializing)
					{
						audio->SetMinMaxDistance(ac.MinDistance, ac.MaxDistance);
						audio->SetPosition(tc.Translation);
					}
				}
			}
			Renderer2D::End();

			bool isMainCameraListening = false;
			const auto& audioListenerView = m_Registry.view<TransformComponent, AudioListenerComponent>();
			for (const auto entity : audioListenerView)
			{
				auto& [tc, al] = audioListenerView.get<TransformComponent, AudioListenerComponent>(entity);
				if(al.Enable)
					al.Listener.Set(tc.Translation, glm::vec3(0.0f), tc.GetForward(), tc.GetUp());
				isMainCameraListening = al.Enable;
			}

			if(!isMainCameraListening)
				editorCamera.UpdateAudioListener(ts);

			m_PhysicsScene->Simulate(ts);
			m_Physics2D->Simulate(ts);
		}

		UpdateEditorTransform();
		RenderScene(editorCamera);
	}

	void Scene::OnSimulationStart()
	{
		PROFILER_SCENE();

		m_Running = true;

		ScriptEngine::SetSceneContext(this);
		const auto &scriptView = m_Registry.view<ScriptComponent>();

		for (auto e : scriptView)
		{
			Entity entity = { e, this };
			ScriptEngine::OnCreateEntity(entity);
		}

		m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
		{
				nsc.Instance = nsc.InstantiateScript();
				nsc.Instance->m_Entity = Entity { entity, this };
		});

		m_PhysicsScene->OnSimulationStart();
		m_Physics2D->OnSimulationStart();

		// Audio
		const auto& audioView = m_Registry.view<AudioComponent>();
		for (auto& e : audioView)
		{
			auto& ac = audioView.get<AudioComponent>(e);
			std::shared_ptr<AudioSource>& audio = AssetManager::GetAsset<AudioSource>(ac.Audio);

			if (ac.PlayAtStart)
				audio->Play();
		}
	}

	void Scene::OnSimulationStop()
	{
		PROFILER_SCENE();

		m_Running = false;

		ScriptEngine::ClearSceneContext();
		m_Registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
		{
			nsc.DestroyScript(&nsc);
		});

		m_PhysicsScene->OnSimulationStop();
		m_Physics2D->OnSimulationStop();

		// Audio
		auto view = m_Registry.view<AudioComponent>();
		for (auto& e : view)
		{
			auto& ac = view.get<AudioComponent>(e);
			
			if (const std::shared_ptr<AudioSource>& audio = AssetManager::GetAsset<AudioSource>(ac.Audio))
				audio->Stop();
		}
	}

	void Scene::RenderScene(const SceneCamera& camera, const TransformComponent& cameraTransform)
	{
		PROFILER_RENDERING();

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		std::sort(m_EntityStorage.begin(), m_EntityStorage.end(), [&](const auto a, const auto b)
		{
			Entity eA { std::get<1>(a), this };
			Entity eB { std::get<1>(b), this };

			float aLen = eA.GetComponent<TransformComponent>().WorldTranslation.z; //glm::length(camera.GetPosition() - eA.GetComponent<TransformComponent>().Translation);
			float bLen = eB.GetComponent<TransformComponent>().WorldTranslation.z; //glm::length(camera.GetPosition() - eB.GetComponent<TransformComponent>().Translation);
			return aLen < bLen;
		});

		Renderer2D::Begin(camera, cameraTransform.GetTransform());
		glDisable(GL_DEPTH_TEST);
		// Render All entities
		for (auto e : m_EntityStorage)
		{
			Entity entity { std::get<1>(e), this };
			auto &tc = entity.GetComponent<TransformComponent>();

			// 2D Quads
			if (entity.HasComponent<SpriteRenderer2DComponent>())
			{
				auto &src = entity.GetComponent<SpriteRenderer2DComponent>();
				if (entity.HasComponent<SpriteAnimationComponent>())
				{
					auto &ac = entity.GetComponent<SpriteAnimationComponent>();
					if (ac.State->IsCurrentAnimationExists())
					{
						if (ac.State->GetAnimation()->HasFrame())
						{
							auto &anim = ac.State->GetAnimation();
							src.Texture = anim->GetCurrentFrame().Handle;
							src.Min = anim->GetCurrentFrame().Min;
							src.Max = anim->GetCurrentFrame().Max;
						}
					}
				}
				Renderer2D::DrawSprite(tc.GetTransform(), src, static_cast<int>(std::get<1>(e)));
			}

			if (entity.HasComponent<CircleRendererComponent>())
			{
				auto &cc = entity.GetComponent<CircleRendererComponent>();
				Renderer2D::DrawCircle(tc.GetTransform(), cc.Color, cc.Thickness, cc.Fade,
															 static_cast<int>(std::get<1>(e)));
			}

			// Particles
			if (entity.HasComponent<ParticleComponent>())
			{
				auto &pc = entity.GetComponent<ParticleComponent>();
				for (int i = 0; i < 5; i++)
				{
					pc.Particle.Emit(pc,
													 glm::vec3(tc.Translation.x, tc.Translation.y, tc.Translation.z + pc.ZAxis),
													 tc.Scale, pc.Rotation, static_cast<int>(std::get<1>(e))
					);
				}

				pc.Particle.OnRender();
			}

			// Text
			if (entity.HasComponent<TextComponent>())
			{
				auto &text = entity.GetComponent<TextComponent>();
				Renderer2D::DrawString(text.TextString, tc.GetTransform(), text, static_cast<int>(std::get<1>(e)));
			}
		}

		Renderer2D::End();
		glEnable(GL_DEPTH_TEST);

		auto lightView = m_Registry.view<TransformComponent, LightComponent>();
		auto meshView = m_Registry.view<TransformComponent, StaticMeshComponent>();

		for (auto entity : meshView)
		{
			auto &[tc, mesh] = meshView.get<TransformComponent, StaticMeshComponent>(entity);

			if (AssetManager::GetAssetType(mesh.Handle) == AssetType::StaticMesh)
			{
				std::shared_ptr<Model> model = AssetManager::GetAsset<Model>(mesh.Handle);

				for (auto& light : lightView)
				{
					auto& [lTC, lc] = lightView.get<TransformComponent, LightComponent>(light);
					lc.Light->GetShadow()->OnAttachTexture(model->GetMaterial()->m_Shader);
					lc.Light->OnRender(lTC);
				}

				model->SetTransform(tc.GetTransform());
				model->Draw(static_cast<int>(entity));
			}
		}

		glDisable(GL_CULL_FACE);
	}

	void Scene::RenderScene(const EditorCamera& camera)
	{
		PROFILER_RENDERING();

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		std::sort(m_EntityStorage.begin(), m_EntityStorage.end(), [&](const auto a, const auto b)
		{
			Entity eA { std::get<1>(a), this };
			Entity eB { std::get<1>(b), this };

			float aLen = eA.GetComponent<TransformComponent>().WorldTranslation.z; //glm::length(camera.GetPosition() - eA.GetComponent<TransformComponent>().Translation);
			float bLen = eB.GetComponent<TransformComponent>().WorldTranslation.z; //glm::length(camera.GetPosition() - eB.GetComponent<TransformComponent>().Translation);
			return aLen < bLen;
		});

		glDisable(GL_DEPTH_TEST);
		Renderer2D::Begin(camera);

		// Render All entities
		for (auto e : m_EntityStorage)
		{
			Entity entity { std::get<1>(e), this };
			auto &tc = entity.GetComponent<TransformComponent>();

			// 2D Quads
			if (entity.HasComponent<SpriteRenderer2DComponent>())
			{
				auto &src = entity.GetComponent<SpriteRenderer2DComponent>();
				if (entity.HasComponent<SpriteAnimationComponent>())
				{
					auto &ac = entity.GetComponent<SpriteAnimationComponent>();
					if (ac.State->IsCurrentAnimationExists())
					{
						if (ac.State->GetAnimation()->HasFrame())
						{
							auto &anim = ac.State->GetAnimation();
							src.Texture = anim->GetCurrentFrame().Handle;
							src.Min = anim->GetCurrentFrame().Min;
							src.Max = anim->GetCurrentFrame().Max;
						}
					}
				}
				Renderer2D::DrawSprite(tc.GetTransform(), src, static_cast<int>(std::get<1>(e)));
			}

			if (entity.HasComponent<CircleRendererComponent>())
			{
				auto &cc = entity.GetComponent<CircleRendererComponent>();
				Renderer2D::DrawCircle(tc.GetTransform(), cc.Color, cc.Thickness, cc.Fade,
															 static_cast<int>(std::get<1>(e)));
			}

			// Particles
			if (entity.HasComponent<ParticleComponent>())
			{
				auto &pc = entity.GetComponent<ParticleComponent>();
				for (int i = 0; i < 5; i++)
				{
					pc.Particle.Emit(pc,
													 glm::vec3(tc.Translation.x, tc.Translation.y, tc.Translation.z + pc.ZAxis),
													 tc.Scale, pc.Rotation, static_cast<int>(std::get<1>(e))
					);
				}

				pc.Particle.OnRender();
			}

			// Text
			if (entity.HasComponent<TextComponent>())
			{
				auto &text = entity.GetComponent<TextComponent>();
				Renderer2D::DrawString(text.TextString, tc.GetTransform(), text, static_cast<int>(std::get<1>(e)));
			}
		}

		Renderer2D::End();
		glEnable(GL_DEPTH_TEST);

		const auto& lightView = m_Registry.view<TransformComponent, LightComponent>();
		const auto& meshView = m_Registry.view<TransformComponent, StaticMeshComponent>();
		for (auto& entity : meshView)
		{
			auto& [tc, mesh] = meshView.get<TransformComponent, StaticMeshComponent>(entity);

			if (AssetManager::GetAssetType(mesh.Handle) == AssetType::StaticMesh)
			{
				std::shared_ptr<Model> model = AssetManager::GetAsset<Model>(mesh.Handle);

				for (auto& light : lightView)
				{
					auto& [lightTransform, lc] = lightView.get<TransformComponent, LightComponent>(light);
					lc.Light->GetShadow()->OnAttachTexture(model->GetMaterial()->m_Shader);
					lc.Light->OnRender(lightTransform);
				}

				model->SetTransform(tc.GetTransform());
				model->Draw((int)entity);
			}
		}

		glDisable(GL_CULL_FACE);
	}
	
	void Scene::OnShadowRender()
	{
		PROFILER_RENDERING();

		const auto& dirLight = m_Registry.view<TransformComponent, LightComponent>();
		for (auto& light : dirLight)
		{
			auto& [lightTransform, lc] = dirLight.get<TransformComponent, LightComponent>(light);
			lc.Light->GetShadow()->BindFramebuffer();

			const auto& meshView = m_Registry.view<TransformComponent, StaticMeshComponent>();
			for (auto& entity : meshView)
			{
				auto& [tc, mesh] = meshView.get<TransformComponent, StaticMeshComponent>(entity);

				if (AssetManager::GetAssetType(mesh.Handle) == AssetType::StaticMesh)
				{
					std::shared_ptr<Model> model = AssetManager::GetAsset<Model>(mesh.Handle);
					lc.Light->GetShadow()->OnRenderBegin(lightTransform, tc.GetTransform());
					model->SetTransform(tc.GetTransform());
					model->Draw();
					lc.Light->GetShadow()->OnRenderEnd();
				}
			}

			lc.Light->GetShadow()->UnbindFramebuffer();
		}
	}

	void Scene::UpdateEditorTransform()
	{
		PROFILER_FUNCTION();

		auto &view = m_Registry.view<IDComponent, TransformComponent>();
		for (auto entity : view)
		{
			auto &[idc, tc] = view.get<IDComponent, TransformComponent>(entity);

			if (idc.Parent != 0)
			{
				auto &ptc = GetEntityWithUUID(idc.Parent).GetComponent<TransformComponent>();
				glm::vec3 rotatedLocalPos = glm::rotate(glm::quat(ptc.WorldRotation), tc.Translation);
				tc.WorldTranslation = rotatedLocalPos + ptc.WorldTranslation;
				tc.WorldRotation = ptc.WorldRotation + tc.Rotation;
				tc.WorldScale = tc.Scale * ptc.WorldScale;
			}
			else
			{
				tc.WorldTranslation = tc.Translation;
				tc.WorldRotation = tc.Rotation;
				tc.WorldScale = tc.Scale;
			}
		}
	}

	void Scene::OnViewportResize(const uint32_t width, const uint32_t height)
	{
		PROFILER_SCENE();

		const auto& view = m_Registry.view<CameraComponent>();
		for (auto& e : view)
		{
			auto& cc = view.get<CameraComponent>(e);
			if(cc.Primary)
				cc.Camera.SetViewportSize(width, height);
		}

		m_ViewportWidth = width;
		m_ViewportHeight = height;
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
	OGN_REG_COMPONENT(TagComponent)
	OGN_REG_COMPONENT(TransformComponent)
	OGN_REG_COMPONENT(AudioComponent)
	OGN_REG_COMPONENT(AudioListenerComponent)
	OGN_REG_COMPONENT(SpriteAnimationComponent)
	OGN_REG_COMPONENT(SpriteRenderer2DComponent)
	OGN_REG_COMPONENT(LightComponent)
	OGN_REG_COMPONENT(StaticMeshComponent)
	OGN_REG_COMPONENT(TextComponent)
	OGN_REG_COMPONENT(CircleRendererComponent)
	OGN_REG_COMPONENT(NativeScriptComponent)
	OGN_REG_COMPONENT(ScriptComponent)
	OGN_REG_COMPONENT(Rigidbody2DComponent)
	OGN_REG_COMPONENT(BoxCollider2DComponent)
	OGN_REG_COMPONENT(CircleCollider2DComponent)
	OGN_REG_COMPONENT(RevoluteJoint2DComponent)
	OGN_REG_COMPONENT(ParticleComponent)
	OGN_REG_COMPONENT(RigidbodyComponent)
	OGN_REG_COMPONENT(BoxColliderComponent)
	OGN_REG_COMPONENT(SphereColliderComponent)
	OGN_REG_COMPONENT(CapsuleColliderComponent)

	template <>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
			component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}
}
