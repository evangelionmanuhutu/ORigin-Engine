// Copyright (c) Evangelion Manuhutu | ORigin Engine
#pragma once

#include "Origin/Animation/AnimationState.h"
#include "Origin/Audio/AudioListener.h"
#include "Origin/Math/Math.h"
#include "Origin/Core/UUID.h"
#include "Origin/Renderer/Texture.h"
#include "Origin/Renderer/Model.h"
#include "Origin/Renderer/Font.h"
#include "Origin/Renderer/ParticleSystem.h"
#include "Origin/Renderer/Material.h"
#include "Origin/Renderer/Framebuffer.h"
#include "Origin/Renderer/MeshVertexData.h"

#include "Origin/Scene/Camera.h"
#include "Origin/Scene/SceneCamera.h"
#include "Origin/Scene/SpriteSheet.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>

namespace origin
{
	class Lighting;
	class AudioSource;
	class SpriteAnimation;
    class ScriptableEntity;
	class RigidbodyComponent;
    class BoxColliderComponent;
    class SphereColliderComponent;
    class CapsuleColliderComponent;

	class IDComponent
	{
	public:
		UUID ID;
		UUID Parent = UUID(0);

		IDComponent() = default;
		IDComponent(const IDComponent &) = default;
		IDComponent(UUID id, UUID parent = UUID(0)) : ID(id), Parent(parent) { }
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
	
	class SpriteAnimationComponent
	{
	public:
		std::shared_ptr<AnimationState<SpriteAnimation>> State;

		SpriteAnimationComponent()
		{
			State = std::make_shared<AnimationState<SpriteAnimation>>();
		}

		SpriteAnimationComponent(const SpriteAnimationComponent&) = default;
		static const AnimationType Type = AnimationType::Sprite;
	};

	class AudioListenerComponent
	{
	public:
		AudioListener Listener;
		uint32_t Index = 0;
		bool Enable = true;
		AudioListenerComponent() = default;
		AudioListenerComponent(const AudioListenerComponent&) = default;
	};
	
	class AudioComponent
	{
	public:
		AssetHandle Audio = UUID(0);
		std::string Name;
		float Volume = 1.0f;
		float Panning = 0.0f;
		float Pitch = 1.0f;
		float MinDistance = 10.0f;
		float MaxDistance = 20.0f;
		bool Spatializing = false;
		bool Looping = false;
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
		enum class Type
		{
			Cube = 0,
			Sphere,
			Capsule
		};

		std::string Name;
		MeshData Data;
		AssetHandle HMaterial = UUID(0);
		Type mType;

		StaticMeshComponent() = default;
		StaticMeshComponent(const StaticMeshComponent&) = default;
	};

	class ModelComponent
	{
	public:
		AssetHandle Handle = UUID(0);
		ModelComponent() = default;
		ModelComponent(const ModelComponent &) = default;
	};

	class TextComponent
	{
	public:
		AssetHandle FontHandle = UUID(0);
		std::string TextString = "This is text component";

		glm::vec4 Color = glm::vec4(1.0f);
		float Kerning = 0.0f;
		float LineSpacing = 0.0f;

		bool ScreenSpace = false;

		TextComponent() = default;
		TextComponent(const TextComponent &) = default;
	};

	class TransformComponent
	{
	public:
		glm::vec3 Translation = glm::vec3(0.0f);
		glm::vec3 Rotation = glm::vec3(0.0f);
		glm::vec3 Scale = glm::vec3(1.0f);
		glm::vec3 WorldTranslation = glm::vec3(0.0f);
		glm::vec3 WorldRotation = glm::vec3(0.0f);
		glm::vec3 WorldScale = glm::vec3(1.0f);

		bool Visible = true;

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;

		TransformComponent(const glm::vec3& translation)
			: Translation(translation)
		{
		}

		void SetTransform(glm::mat4 transform)
		{
			Math::DecomposeTransformEuler(transform, WorldTranslation, WorldRotation, WorldScale);
		}

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(WorldRotation));
			return glm::translate(glm::mat4(1.0f), WorldTranslation)
				* rotation * glm::scale(glm::mat4(1.0f), WorldScale);
		}

		glm::vec3 GetForward() const
		{
			glm::mat4 rotation = toMat4(glm::quat(WorldRotation));
			glm::vec4 forward = rotation * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
			return glm::normalize(glm::vec3(forward));
		}

		glm::vec3 GetUp() const
		{
			glm::mat4 rotation = toMat4(glm::quat(WorldRotation));
			glm::vec4 up = rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
			return glm::normalize(glm::vec3(up));
		}

		glm::vec3 GetRight() const
		{
			glm::mat4 rotation = toMat4(glm::quat(WorldRotation));
			glm::vec4 right = rotation * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
			return glm::normalize(glm::vec3(right));
		}
	};

	class SpriteRenderer2DComponent
	{
	public:
		AssetHandle Texture = UUID(0);

		glm::vec4 Color = glm::vec4(1.0f);
		glm::vec2 Min = glm::vec2(0.0f, 0.0f);
		glm::vec2 Max = glm::vec2(1.0f, 1.0f);
		glm::vec2 TillingFactor = glm::vec2(1.0f);

		bool FlipX = false;
		bool FlipY = false;

		SpriteRenderer2DComponent() = default;
		SpriteRenderer2DComponent(const SpriteRenderer2DComponent&) = default;
		SpriteRenderer2DComponent(float r, float g, float b, float a)
			: Color(r, g, b, a){}
	};
	
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
		glm::vec2 Offset = {0.0f, 0.0f};
		glm::vec2 Size = {0.5f, 0.5f};

		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;

		void* RuntimeFixture = nullptr;
		void* RuntimeBoxShape = nullptr;

		// All fixtures with the same group index always collide (positive index)
		// or never collide(negative index)
		int Group = 1;

		BoxCollider2DComponent() = default;
		BoxCollider2DComponent(const BoxCollider2DComponent&) = default;
	};

	class CircleCollider2DComponent
	{
	public:

		glm::vec2 Offset = {0.0f, 0.0f};
		float Radius = 0.5f;

		float Density = 1.0f;
		float Friction = 0.5f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 0.5f;

		void* RuntimeFixture = nullptr;
		void* RuntimeCircleShape = nullptr;

		// All fixtures with the same group index always collide(positive index)
		// or never collide(negative index)
		int Group = 1;

		CircleCollider2DComponent() = default;
		CircleCollider2DComponent(const CircleCollider2DComponent&) = default;
	};

	class RevoluteJoint2DComponent
	{
	public:
		glm::vec2 AnchorPoint = glm::vec2(0.0f);

		// Angles in degrees 
		// (it is converted to radians when creating component)
		bool EnableLimit = true;
		float LowerAngle = 90.0f;
		float UpperAngle = 45.0f;
		float MaxMotorTorque = 10.0f;
		bool EnableMotor = true;
		float MotorSpeed = 0.0f;
		void* Joint = nullptr;

		UUID ConnectedBodyID = 0;

		RevoluteJoint2DComponent() = default;
		RevoluteJoint2DComponent(const RevoluteJoint2DComponent&) = default;
	};

	struct BaseUIData
	{
		virtual ~BaseUIData() = default;
		enum class Anchor
		{
			Center,
			Left, Right,
			TopLeft, TopRight,
			BottomLeft, BottomRight
		};

		std::string Name;
		Anchor AnchorType;
		TransformComponent Transform;

		BaseUIData() = default;
		BaseUIData(const std::string &name, Anchor anchorType = Anchor::Center)
			: Name(name), AnchorType(anchorType)
		{
		}

	};

	template<typename T>
	struct UIData : public BaseUIData
	{
		T Component;

		UIData() = default;
		UIData(const std::string &name, const T &component, BaseUIData::Anchor anchorType = BaseUIData::Anchor::Center)
			: BaseUIData(name, anchorType), Component(component)
		{
		}
	};

	class UIComponent
	{
	public:
		UIComponent() = default;

		template<typename T>
		void AddComponent(const std::string &name, UIData<T> component)
		{
			component.Name = GenerateUniqueKey(name);
			Components.push_back(std::make_shared<UIData<T>>(component));
		}

		template<typename T>
		UIData<T> *GetComponent(const std::string &name)
		{
			for (auto &c : Components)
			{
				if (name == c->Name)
					return dynamic_cast<UIData<T>*>(c.get());
			}
			return nullptr;
		}

		bool RenameComponent(int index, const std::string &newName)
		{
			if (newName.empty())
				return false;

			bool foundSameName = false;
			for (auto &d : Components)
			{
				foundSameName = d->Name == newName;
				if (foundSameName)
					break;
			}

			if (foundSameName)
			{
				Components[index]->Name = GenerateUniqueKey(newName);
			}
			else
			{
				Components[index]->Name = newName;
			}

			return true;
		}

		void RemoveComponent(int index)
		{
			Components.erase(Components.begin() + index);
		}

		std::vector<std::shared_ptr<BaseUIData>> Components;
		std::unordered_map<std::string, int> ComponentCounters;
		std::shared_ptr<Framebuffer> OFramebuffer;

	private:
		std::string GenerateUniqueKey(const std::string &name)
		{
			// check if the name already exists
			bool found = false;
			for (auto &c : Components)
			{
				if (c->Name == name)
				{
					found = true;
					break;
				}
			}

			// if the name does not exist, return it as the unique key
			if (!found)
				return name;

			// generate a unique key if the name already exists
			int counter = 1;
			std::string uniqueKey;
			bool unique = false;

			while (!unique)
			{
				uniqueKey = name + std::to_string(counter);
				unique = true;
				for (auto &c : Components)
				{
					if (c->Name == uniqueKey)
					{
						unique = false;
						break;
					}
				}
				counter++;
			}

			// update the counter to reflect the highest number used
			ComponentCounters[name] = counter - 1;

			// clean up counters for names that no longer exist in Components
			for (auto it = ComponentCounters.begin(); it != ComponentCounters.end();)
			{
				bool exists = false;
				for (auto &c : Components)
				{
					if (c->Name.find(it->first) == 0)
					{
						exists = true;
						break;
					}
				}

				if (!exists)
					it = ComponentCounters.erase(it);
				else
					++it;
			}

			return uniqueKey;
		}
	};

	template <typename... Component>
	struct ComponentGroup
	{
	};

	using AllComponents = ComponentGroup<
		TransformComponent, 
		CameraComponent,
		UIComponent, 
		SpriteAnimationComponent, 
		AudioComponent, 
		AudioListenerComponent, 
		LightComponent,
		SpriteRenderer2DComponent, 
		StaticMeshComponent, 
		ModelComponent, 
		TextComponent,
		CircleRendererComponent, 
		ParticleComponent, 
		ScriptComponent, 
		NativeScriptComponent,
		Rigidbody2DComponent, 
		BoxCollider2DComponent, 
		CircleCollider2DComponent, 
		RevoluteJoint2DComponent,
        RigidbodyComponent,
        BoxColliderComponent,
        SphereColliderComponent,
        CapsuleColliderComponent>;
}
