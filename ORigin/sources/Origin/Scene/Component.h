// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#pragma once
#include "pch.h"

#include "Origin/Animation/AnimationState.h"
#include "Origin/Math/Math.h"
#include "Origin/Audio/AudioListener.h"

#include "SceneCamera.h"
#include "Origin/Core/UUID.h"

#include "Origin/Renderer/Texture.h"
#include "Origin/Renderer/Model.h"
#include "Origin/Renderer/Font.h"
#include "Origin/Renderer/ParticleSystem.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Origin/Renderer/Material.h"
#include "Origin/Renderer/Framebuffer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>


namespace origin
{
	class Component
	{
	public:
		virtual void Destroy() = 0;
	};

	class IDComponent
	{
	public:
		UUID ID;
		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
	};

	class TagComponent
	{
	public:
		std::string Tag;
		TagComponent() = default;
		TagComponent(const TagComponent&) = default;

		TagComponent(const std::string& tag) : Tag(tag)
		{
		}
	};

	class Animation;

	class AnimationComponent
	{
	public:
		AnimationComponent() = default;
		AnimationComponent(const AnimationComponent&) = default;
		AnimationState State;
	};

	class AudioListenerComponent
	{
	public:
		AudioListener Listener;
		bool Enable = true;
		AudioListenerComponent() = default;
		AudioListenerComponent(const AudioListenerComponent&) = default;
	};

	class Audio;

	class AudioComponent
	{
	public:
		std::shared_ptr<Audio> Audio;
		std::string Name = "Audio";

		float Volume = 1.0f;
		float Pitch = 1.0f;
		float MinDistance = 1.0f;
		float MaxDistance = 100.0f;

		bool Looping = false;
		bool Spatial = false;
		bool PlayAtStart = false;

		AudioComponent() = default;
		AudioComponent(const AudioComponent&) = default;
	};

	class ParticleComponent
	{
	public:
		ParticleSystem Particle;

		glm::vec3 Velocity = glm::vec3(0.0f);
		glm::vec3 VelocityVariation = glm::vec3(1.0f);
		glm::vec3 Rotation = glm::vec3(1.0f);

		glm::vec4 ColorBegin = {254 / 255.0f, 212 / 255.0f, 123 / 255.0f, 1.0f};
		glm::vec4 ColorEnd = {254 / 255.0f, 109 / 255.0f, 41 / 255.0f, 1.0f};
		uint32_t PoolIndex = 1000;

		float SizeBegin = 0.5f;
		float SizeEnd = 0.0f;
		float SizeVariation = 0.3f;
		float ZAxis = 0.0f;
		float LifeTime = 1.0f;

		ParticleComponent() = default;
		ParticleComponent(const ParticleComponent&) = default;
	};

	class StaticMeshComponent
	{
	public:
		std::shared_ptr<Material> Material;
		std::shared_ptr<Model> Model;

		StaticMeshComponent() = default;
		StaticMeshComponent(const StaticMeshComponent&) = default;
	};

	class TextComponent
	{
	public:
		std::string TextString;
		std::shared_ptr<Font> FontAsset;

		glm::vec4 Color = glm::vec4(1.0f);
		float Kerning = 0.0f;
		float LineSpacing = 0.0f;
	};

	class TransformComponent
	{
	public:
		glm::vec3 Translation = glm::vec3(0.0f);
		glm::vec3 Rotation = glm::vec3(0.0f);
		glm::vec3 Scale = glm::vec3(1.0f);

		glm::vec3 WorldPosition = glm::vec3(0.0f);
		glm::quat WorldRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;

		TransformComponent(const glm::vec3& translation)
			: Translation(translation)
		{
		}

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = toMat4(glm::quat(Rotation));
			return translate(glm::mat4(1.0f), Translation)
				* rotation * scale(glm::mat4(1.0f), Scale);
		}

		glm::vec3 GetForward() const
		{
			glm::mat4 rotation = toMat4(glm::quat(Rotation));
			glm::vec4 forward = rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
			return normalize(glm::vec3(forward));
		}

		glm::vec3 GetUp() const
		{
			glm::mat4 rotation = toMat4(glm::quat(Rotation));
			glm::vec4 up = rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
			return normalize(glm::vec3(up));
		}

		glm::vec3 GetRight() const
		{
			glm::mat4 rotation = toMat4(glm::quat(Rotation));
			glm::vec4 right = rotation * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
			return normalize(glm::vec3(right));
		}
	};

	class SpriteRendererComponent
	{
	public:
		glm::vec4 Color = glm::vec4(1.0f);
		std::shared_ptr<Texture2D> Texture;

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;

		SpriteRendererComponent(const glm::vec4& color) : Color(color)
		{
		}

		SpriteRendererComponent(float r, float g, float b, float, float a) : Color(r, g, b, a)
		{
		}
	};

	class SpriteRenderer2DComponent
	{
	public:
		glm::vec4 Color = glm::vec4(1.0f);
		std::shared_ptr<Texture2D> Texture;
		glm::vec2 TillingFactor = glm::vec2(1.0f);

		bool FlipX = false;
		bool FlipY = false;

		SpriteRenderer2DComponent() = default;
		SpriteRenderer2DComponent(const SpriteRenderer2DComponent&) = default;

		SpriteRenderer2DComponent(const SpriteRenderer2DComponent&, glm::vec4 color) : Color(color)
		{
		}

		SpriteRenderer2DComponent(float r, float g, float b, float a) : Color(r, g, b, a)
		{
		}
	};

	class Lighting;
	class LightComponent
	{
	public:
		std::shared_ptr<Lighting> Light;

		LightComponent() = default;
		LightComponent(const LightComponent&) = default;
	};

	class CircleRendererComponent
	{
	public:
		glm::vec4 Color = glm::vec4(1.0f);
		float Thickness = 1.0f;
		float Fade = 0.005f;

		CircleRendererComponent() = default;
		CircleRendererComponent(const CircleRendererComponent&) = default;
	};

	class CameraComponent
	{
	public:
		SceneCamera Camera;
		bool Primary = true;
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	class ScriptComponent
	{
	public:
		std::string ClassName = "None";
		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent&) = default;
	};

	class ScriptableEntity;
	class NativeScriptComponent
	{
	public:
		ScriptableEntity* Instance;
		ScriptableEntity* (*InstantiateScript)();

		void (*DestroyScript)(NativeScriptComponent* nsc);

		template <typename T>
		void Bind()
		{
			InstantiateScript = []() { return static_cast<ScriptableEntity*>(new T()); };
			DestroyScript = [](NativeScriptComponent* nsc)
				{
					delete nsc->Instance;
					nsc->Instance = nullptr;
				};
		}
	};

	class Rigidbody2DComponent
	{
	public:
		void* RuntimeBody = nullptr;

		enum class BodyType { Static = 0, Dynamic, Kinematic };
		BodyType Type = BodyType::Static;

		float Mass = 1.0f;
		float LinearDamping = 0.0f;
		float AngularDamping = 0.01f;
		float RotationalInertia = 0.0f;
		glm::vec2 MassCenter = glm::vec2(0.0f);
		float GravityScale = 1.0f;

		bool FreezePositionX = false;
		bool FreezePositionY = false;
		bool FixedRotation = false;
		bool AllowSleeping = true;
		bool Awake = true;
		bool Bullet = true;
		bool Enabled = true;

		std::string ContactWith;

		Rigidbody2DComponent() = default;
		Rigidbody2DComponent(const Rigidbody2DComponent&) = default;
	};

	class BoxCollider2DComponent
	{
	public:
		// All fixtures with the same group index always collide(positive index)
		// or never collide(negative index)
		int Group = 1;

		glm::vec2 Offset = {0.0f, 0.0f};
		glm::vec2 Size = {0.5f, 0.5f};

		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;

		void* RuntimeFixture = nullptr;
		void* RuntimeBoxShape = nullptr;

		BoxCollider2DComponent() = default;
		BoxCollider2DComponent(const BoxCollider2DComponent&) = default;
	};

	class CircleCollider2DComponent
	{
	public:
		// All fixtures with the same group index always collide(positive index)
		// or never collide(negative index)
		int Group = 1;

		glm::vec2 Offset = {0.0f, 0.0f};
		float Radius = 0.5f;

		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;

		void* RuntimeFixture = nullptr;
		void* RuntimeCircleShape = nullptr;

		CircleCollider2DComponent() = default;
		CircleCollider2DComponent(const CircleCollider2DComponent&) = default;
	};

	template <typename... Component>
	struct ComponentGroup
	{
	};

	class RigidbodyComponent;
	class BoxColliderComponent;
	class SphereColliderComponent;

	using AllComponents = ComponentGroup<
		TransformComponent, CameraComponent, AnimationComponent,
		AudioComponent, AudioListenerComponent, LightComponent,
		SpriteRendererComponent, SpriteRenderer2DComponent, StaticMeshComponent, TextComponent,
		CircleRendererComponent, ParticleComponent, ScriptComponent, NativeScriptComponent,
		Rigidbody2DComponent, BoxCollider2DComponent, CircleCollider2DComponent,
		RigidbodyComponent, BoxColliderComponent, SphereColliderComponent
	>;
}
