// Copyright (c) Evangelion Manuhutu | ORigin Engine

#include "pch.h"

#include "SceneSerializer.h"
#include "Origin/Scripting/ScriptEngine.h"
#include "Origin/Project/Project.h"
#include "Origin/Renderer/Model.h"
#include "Origin/Renderer/Shader.h"
#include "Origin/Renderer/Renderer.h"
#include "Origin/Audio/Audio.h"

#include "Entity.h"
#include "Lighting.h"
#include "Component.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace YAML
{
	template <>
	struct convert<origin::UUID>
	{
		static Node encode(const origin::UUID uuid)
		{
			Node node;
			node.push_back(static_cast<uint64_t>(uuid));
			return node;
		}

		static bool decode(const Node& node, origin::UUID uuid)
		{
			uuid = node.as<uint64_t>();
			return true;
		}
	};

	template <>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template <>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template <>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};
}

namespace origin
{
#define WRITE_FIELD_TYPE(FieldType, Type)\
				case ScriptFieldType::FieldType:\
				{\
					out << scriptField.GetValue<Type>();\
					break;\
				}
#define	READ_FIELD_TYPE(FieldType, Type)\
				case ScriptFieldType::FieldType:\
				{\
					Type data = scriptField["Data"].as<Type>();\
					fieldInstance.SetValue(data);\
					break;\
				}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	static std::string RigidBody2DBodyTypeToString(Rigidbody2DComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
		case Rigidbody2DComponent::BodyType::Static: return "Static";
		case Rigidbody2DComponent::BodyType::Dynamic: return "Dynamic";
		case Rigidbody2DComponent::BodyType::Kinematic: return "Kinematic";
		}

		OGN_CORE_ASSERT(false, "Unknown body type");
		return {};
	}

	static Rigidbody2DComponent::BodyType Rigidbody2DBodyTypeFromString(const std::string& bodyTypeString)
	{
		if (bodyTypeString == "Static") return Rigidbody2DComponent::BodyType::Static;
		if (bodyTypeString == "Dynamic") return Rigidbody2DComponent::BodyType::Dynamic;
		if (bodyTypeString == "Kinematic") return Rigidbody2DComponent::BodyType::Kinematic;

		OGN_CORE_ASSERT(false, "Unknown body type");
		return Rigidbody2DComponent::BodyType::Static;
	}

	SceneSerializer::SceneSerializer(const std::shared_ptr<Scene>& scene)
		: m_Scene(scene)
	{
	}

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		OGN_CORE_ASSERT(entity.HasComponent<IDComponent>(), "");
		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity" << YAML::Value << entity.GetUUID();

		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap; // TagComponent

			const auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // TagComponent
		}

		if (entity.HasComponent<AnimationComponent>())
		{
			auto& ac = entity.GetComponent<AnimationComponent>();

			out << YAML::Key << "AnimationComponent";
			out << YAML::BeginMap; // AnimationComponent
			out << YAML::Key << "DefaultState" << YAML::Value << ac.State.GetDefaultState();

			out << YAML::Key << "States";
			out << YAML::BeginSeq; // States

			// Find States
			for (int stateIndex = 0; stateIndex < ac.State.GetStateStorage().size(); stateIndex++)
			{
				const std::string currentState = ac.State.GetStateStorage().at(stateIndex);

				out << YAML::BeginMap; // Name
				out << YAML::Key << "Name" << currentState;

				if (ac.State.HasAnimation())
				{
					auto animations = ac.State.GetAnimationState().at(currentState);
					{
						out << YAML::Key << "Frames" << YAML::Value;
						out << YAML::BeginSeq; // Frames
						for (int frameIndex = 0; frameIndex < animations.GetTotalFrames(); frameIndex++)
						{
							out << YAML::BeginMap; // Path
							std::filesystem::path& framePath = relative(animations.GetSprites(frameIndex)->GetFilepath(), Project::GetAssetDirectory());
							out << YAML::Key << "Path" << YAML::Value << framePath.generic_string(); // Add the frame path directly to the sequence
							out << YAML::EndMap; //!Path
						}
						out << YAML::EndSeq; //!Frames
					}
					out << YAML::Key << "Looping" << YAML::Value << ac.State.IsLooping();
				}
				out << YAML::EndMap; // !Name
			}
			out << YAML::EndSeq; // !States

			out << YAML::EndMap; // !AnimationComponent
		}


		if (entity.HasComponent<ScriptComponent>())
		{
			out << YAML::Key << "ScriptComponent";
			out << YAML::BeginMap; // ScriptComponent;

			auto& sc = entity.GetComponent<ScriptComponent>();
			out << YAML::Key << "ClassName" << YAML::Value << sc.ClassName;

			// Fields
			const std::shared_ptr<ScriptClass> entityClass = ScriptEngine::GetEntityClass(sc.ClassName);
			const auto& fields = entityClass->GetFields();

			if (!fields.empty())
			{
				out << YAML::Key << "ScriptFields" << YAML::Value;
				auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);

				out << YAML::BeginSeq;
				for (const auto& [name, field] : fields)
				{
					if (entityFields.find(name) == entityFields.end())
					{
						OGN_CORE_ERROR("SceneSerializer: {} SCRIPT FIELDS NOT FOUND", name);
						continue;
					}

					out << YAML::BeginMap; // Fields
					out << YAML::Key << "Name" << YAML::Value << name;
					out << YAML::Key << "Type" << YAML::Value << Utils::ScriptFieldTypeToString(field.Type);
					out << YAML::Key << "Data" << YAML::Value;

					ScriptFieldInstance& scriptField = entityFields.at(name);
					switch (field.Type)
					{
						WRITE_FIELD_TYPE(Float, float);
						WRITE_FIELD_TYPE(Double, double);
						WRITE_FIELD_TYPE(Bool, bool);
						WRITE_FIELD_TYPE(Char, char);
						WRITE_FIELD_TYPE(Byte, int8_t);
						WRITE_FIELD_TYPE(Short, int16_t);
						WRITE_FIELD_TYPE(Int, int32_t);
						WRITE_FIELD_TYPE(Long, int64_t);
						WRITE_FIELD_TYPE(UByte, uint8_t);
						WRITE_FIELD_TYPE(UShort, uint16_t);
						WRITE_FIELD_TYPE(UInt, uint32_t);
						WRITE_FIELD_TYPE(ULong, uint64_t);
						WRITE_FIELD_TYPE(Vector2, glm::vec2);
						WRITE_FIELD_TYPE(Vector3, glm::vec3);
						WRITE_FIELD_TYPE(Vector4, glm::vec4);
						WRITE_FIELD_TYPE(Entity, UUID);
					}

					out << YAML::EndMap; // !Fields
				}
				out << YAML::EndSeq;
			}

			out << YAML::EndMap; // !ScriptComponent;
		}


		if (entity.HasComponent<AudioComponent>())
		{
			out << YAML::Key << "AudioComponent";
			out << YAML::BeginMap; // AudioComponent

			const auto& ac = entity.GetComponent<AudioComponent>();
			out << YAML::Key << "Name" << YAML::Value << ac.Name;

			auto& audioPath = relative(ac.Audio->GetFilepath(), Project::GetAssetDirectory());
			out << YAML::Key << "Filepath" << YAML::Value << audioPath.generic_string();
			out << YAML::Key << "Volume" << YAML::Value << ac.Volume;
			out << YAML::Key << "Pitch" << YAML::Value << ac.Pitch;
			out << YAML::Key << "MinDistance" << YAML::Value << ac.MinDistance;
			out << YAML::Key << "MaxDistance" << YAML::Value << ac.MaxDistance;
			out << YAML::Key << "Looping" << YAML::Value << ac.Looping;
			out << YAML::Key << "Spatial" << YAML::Value << ac.Spatial;
			out << YAML::Key << "PlayAtStart" << YAML::Value << ac.PlayAtStart;

			out << YAML::EndMap; // AudioComponent
		}

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent

			const auto& tc = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << tc.Scale;

			out << YAML::EndMap; // !TransformComponent
		}

		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap; // CameraComponent

			const auto& cameraComponent = entity.GetComponent<CameraComponent>();
			const auto& camera = cameraComponent.Camera;

			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap; // Camera
			out << YAML::Key << "ProjectionType" << YAML::Value << static_cast<int>(camera.GetProjectionType());
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetPerspectiveFov();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.GetPerspectiveNearClip();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.GetPerspectiveFarClip();
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap; // !Camera

			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;
			out << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComponent.FixedAspectRatio;

			out << YAML::EndMap; // !CameraComponent
		}

		if (entity.HasComponent<AudioListenerComponent>())
		{
			out << YAML::Key << "AudioListenerComponent";
			out << YAML::BeginMap; // AudioListenerComponent

			const auto& al = entity.GetComponent<AudioListenerComponent>();
			out << YAML::Key << "Enable" << YAML::Value << al.Enable;

			out << YAML::EndMap; // !AudioListenerComponent
		}

		if (entity.HasComponent<StaticMeshComponent>())
		{
			out << YAML::Key << "StaticMeshComponent";
			out << YAML::BeginMap; // StaticMeshComponent

			const auto& sMesh = entity.GetComponent<StaticMeshComponent>();

			if (sMesh.Model)
			{
				std::filesystem::path modelFilepath = relative(sMesh.Model->GetFilepath(), Project::GetAssetDirectory());
				out << YAML::Key << "ModelPath" << YAML::Value << modelFilepath.generic_string();
				out << YAML::Key << "ShaderPath" << YAML::Value << sMesh.Material->GetShaderFilepath();
				out << YAML::Key << "MaterialName" << YAML::Value << sMesh.Material->GetMaterialName();
				out << YAML::Key << "Color" << YAML::Value << sMesh.Material->Color;
				out << YAML::Key << "Shininess" << YAML::Value << sMesh.Material->Shininess;
				out << YAML::Key << "Bias" << YAML::Value << sMesh.Material->Bias;

				std::filesystem::path texturePath = "";
				auto tilingFactor = glm::vec2(1.0f);

				if (sMesh.Material->Texture)
				{
					texturePath = relative(sMesh.Material->Texture->GetFilepath(), Project::GetAssetDirectory());
					tilingFactor = sMesh.Material->TilingFactor;
				}

				out << YAML::Key << "TextureFilepath" << YAML::Value << texturePath.generic_string();
				out << YAML::Key << "TilingFactor" << YAML::Value << tilingFactor;
			}

			out << YAML::EndMap; // !StaticMeshComponent
		}

		if (entity.HasComponent<BoxColliderComponent>())
		{
			out << YAML::Key << "BoxColliderComponent";
			out << YAML::BeginMap; // BoxColliderComponent
			const auto& boxCollider = entity.GetComponent<BoxColliderComponent>();
			out << YAML::Key << "Size" << YAML::Value << boxCollider.Size;
			out << YAML::Key << "Offset" << YAML::Value << boxCollider.Offset;
			out << YAML::Key << "Restitution" << YAML::Value << boxCollider.Restitution;
			out << YAML::Key << "StaticFriction" << YAML::Value << boxCollider.StaticFriction;
			out << YAML::Key << "DynamicFriction" << YAML::Value << boxCollider.DynamicFriction;

			out << YAML::EndMap; // !BoxColliderComponent
		}

		if (entity.HasComponent<SphereColliderComponent>())
		{
			out << YAML::Key << "SphereColliderComponent";
			out << YAML::BeginMap; // SphereColliderComponent
			const auto& circlerCollider = entity.GetComponent<SphereColliderComponent>();
			out << YAML::Key << "Radius" << YAML::Value << circlerCollider.Radius;
			out << YAML::Key << "Offset" << YAML::Value << circlerCollider.Offset;
			out << YAML::Key << "Restitution" << YAML::Value << circlerCollider.Restitution;
			out << YAML::Key << "StaticFriction" << YAML::Value << circlerCollider.StaticFriction;
			out << YAML::Key << "DynamicFriction" << YAML::Value << circlerCollider.DynamicFriction;

			out << YAML::EndMap; // !SphereColliderComponent
		}

		if (entity.HasComponent<RigidbodyComponent>())
		{
			out << YAML::Key << "RigidbodyComponent";
			out << YAML::BeginMap; // RigidbodyComponent
			const auto& rigidbody = entity.GetComponent<RigidbodyComponent>();

			out << YAML::Key << "Mass" << YAML::Value << rigidbody.Mass;
			out << YAML::Key << "CenterMassPosition" << YAML::Value << rigidbody.CenterMassPosition;

			out << YAML::Key << "UseGravity" << YAML::Value << rigidbody.UseGravity;
			out << YAML::Key << "RotateX" << YAML::Value << rigidbody.RotateX;
			out << YAML::Key << "RotateY" << YAML::Value << rigidbody.RotateY;
			out << YAML::Key << "RotateZ" << YAML::Value << rigidbody.RotateZ;
			out << YAML::Key << "MoveX" << YAML::Value << rigidbody.MoveX;
			out << YAML::Key << "MoveY" << YAML::Value << rigidbody.MoveY;
			out << YAML::Key << "MoveZ" << YAML::Value << rigidbody.MoveZ;
			out << YAML::Key << "Kinematic" << YAML::Value << rigidbody.Kinematic;
			out << YAML::Key << "RetainAcceleration" << YAML::Value << rigidbody.RetainAcceleration;

			out << YAML::EndMap; // !Rigidbody
		}

		if (entity.HasComponent<TextComponent>())
		{
			out << YAML::Key << "TextComponent";
			out << YAML::BeginMap; // TextComponent
			const auto& textComponent = entity.GetComponent<TextComponent>();
			out << YAML::Key << "TextString" << YAML::Value << textComponent.TextString;
			const auto& textAssetPath = relative(textComponent.FontAsset->GetFilepath(), Project::GetAssetDirectory());
			out << YAML::Key << "FontFilepath" << YAML::Value << textAssetPath.generic_string();
			out << YAML::Key << "Color" << YAML::Value << textComponent.Color;
			out << YAML::Key << "Kerning" << YAML::Value << textComponent.Kerning;
			out << YAML::Key << "LineSpacing" << YAML::Value << textComponent.LineSpacing;

			out << YAML::EndMap; // !TextComponent
		}

		if (entity.HasComponent<ParticleComponent>())
		{
			out << YAML::Key << "ParticleComponent";
			out << YAML::BeginMap; // ParticleComponent
			const auto& particleComponent = entity.GetComponent<ParticleComponent>();
			out << YAML::Key << "Velocity" << YAML::Value << particleComponent.Velocity;
			out << YAML::Key << "VelocityVariation" << YAML::Value << particleComponent.VelocityVariation;
			out << YAML::Key << "Rotation" << YAML::Value << particleComponent.Rotation;
			out << YAML::Key << "ColorBegin" << YAML::Value << particleComponent.ColorBegin;
			out << YAML::Key << "ColorEnd" << YAML::Value << particleComponent.ColorEnd;
			out << YAML::Key << "SizeBegin" << YAML::Value << particleComponent.SizeBegin;
			out << YAML::Key << "SizeEnd" << YAML::Value << particleComponent.SizeEnd;
			out << YAML::Key << "SizeVariation" << YAML::Value << particleComponent.SizeVariation;
			out << YAML::Key << "ZAxis" << YAML::Value << particleComponent.ZAxis;
			out << YAML::Key << "LifeTime" << YAML::Value << particleComponent.LifeTime;

			out << YAML::EndMap; // !ParticleComponent
		}

		if (entity.HasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::BeginMap; // SpriteRendererComponent

			const auto& src = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << src.Color;
			if (src.Texture)
			{
				auto& texturePath = relative(src.Texture->GetFilepath(), Project::GetAssetDirectory());
				out << YAML::Key << "TexturePath" << YAML::Value << texturePath.generic_string();
			}

			out << YAML::EndMap; // !SpriteRendererComponent
		}

		if (entity.HasComponent<SpriteRenderer2DComponent>())
		{
			out << YAML::Key << "SpriteRenderer2DComponent";
			out << YAML::BeginMap; // SpriteRenderer2DComponent

			const auto& src = entity.GetComponent<SpriteRenderer2DComponent>();
			out << YAML::Key << "Color" << YAML::Value << src.Color;
			if (src.Texture)
			{
				auto& texturePath = relative(src.Texture->GetFilepath(), Project::GetAssetDirectory());
				out << YAML::Key << "TexturePath" << YAML::Value << texturePath.generic_string();
			}
			out << YAML::Key << "TillingFactor" << YAML::Value << src.TillingFactor;

			out << YAML::EndMap; // !SpriteRenderer2DComponent
		}

		if (entity.HasComponent<LightComponent>())
		{
			out << YAML::Key << "LightComponent";
			out << YAML::BeginMap;
			const auto& light = entity.GetComponent<LightComponent>().Light;
			out << YAML::Key << "Type" << YAML::Value << light->GetTypeString();
			out << YAML::Key << "Color" << YAML::Value << light->Color;

			switch (light->Type)
			{
			case LightingType::Spot:
			{
				out << YAML::Key << "InnerConeAngle" << YAML::Value << light->InnerConeAngle;
				out << YAML::Key << "OuterConeAngle" << YAML::Value << light->OuterConeAngle;
				out << YAML::Key << "Exponent" << YAML::Value << light->Exponent;
				break;
			}
			case LightingType::Point:
			{
				out << YAML::Key << "Ambient" << YAML::Value << light->Ambient;
				out << YAML::Key << "Specular" << YAML::Value << light->Specular;
				break;
			}
			case LightingType::Directional:
			{
				out << YAML::Key << "Ambient" << YAML::Value << light->Ambient;
				out << YAML::Key << "Diffuse" << YAML::Value << light->Diffuse;
				out << YAML::Key << "Specular" << YAML::Value << light->Specular;
				out << YAML::Key << "Near" << YAML::Value << light->Near;
				out << YAML::Key << "Far" << YAML::Value << light->Far;
				out << YAML::Key << "Size" << YAML::Value << light->Size;
				break;
			}
			default:
				break;
			}

			out << YAML::EndMap;
		}

		if (entity.HasComponent<CircleRendererComponent>())
		{
			out << YAML::Key << "CircleRendererComponent";
			out << YAML::BeginMap; // CircleRendererComponent

			const auto& src = entity.GetComponent<CircleRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << src.Color;
			out << YAML::Key << "Fade" << YAML::Value << src.Fade;
			out << YAML::Key << "Thickness" << YAML::Value << src.Thickness;

			out << YAML::EndMap; // !CircleRendererComponent
		}

		if (entity.HasComponent<Rigidbody2DComponent>())
		{
			out << YAML::Key << "Rigidbody2DComponent";
			out << YAML::BeginMap; // Rigidbody2DComponent

			const auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << RigidBody2DBodyTypeToString(rb2d.Type);
			out << YAML::Key << "Mass" << YAML::Value << rb2d.Mass;
			out << YAML::Key << "LinearDamping" << YAML::Value << rb2d.LinearDamping;
			out << YAML::Key << "AngularDamping" << YAML::Value << rb2d.AngularDamping;
			out << YAML::Key << "RotationalInertia" << YAML::Value << rb2d.RotationalInertia;
			out << YAML::Key << "GravityScale" << YAML::Value << rb2d.GravityScale;
			out << YAML::Key << "MassCenter" << YAML::Value << rb2d.MassCenter;
			out << YAML::Key << "FreezePositionX" << YAML::Value << rb2d.FreezePositionX;
			out << YAML::Key << "FreezePositionY" << YAML::Value << rb2d.FreezePositionY;
			out << YAML::Key << "AllowSleeping" << YAML::Value << rb2d.AllowSleeping;
			out << YAML::Key << "Awake" << YAML::Value << rb2d.Awake;
			out << YAML::Key << "Bullet" << YAML::Value << rb2d.Bullet;
			out << YAML::Key << "Enabled" << YAML::Value << rb2d.Enabled;
			out << YAML::Key << "FixedRotation" << YAML::Value << rb2d.FixedRotation;


			out << YAML::EndMap; // !Rigidbody2DComponent
		}

		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			out << YAML::Key << "BoxCollider2DComponent";
			out << YAML::BeginMap; // BoxCollider2DComponent;

			const auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();
			out << YAML::Key << "Group" << YAML::Value << bc2d.Group;
			out << YAML::Key << "Offset" << YAML::Value << bc2d.Offset;
			out << YAML::Key << "Size" << YAML::Value << bc2d.Size;

			out << YAML::Key << "Density" << YAML::Value << bc2d.Density;
			out << YAML::Key << "Friction" << YAML::Value << bc2d.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << bc2d.Restitution;
			out << YAML::Key << "RestitutionThreshold" << YAML::Value << bc2d.RestitutionThreshold;

			out << YAML::EndMap; // !BoxCollider2DComponent
		}

		if (entity.HasComponent<CircleCollider2DComponent>())
		{
			out << YAML::Key << "CircleCollider2DComponent";
			out << YAML::BeginMap; // CircleCollider2DComponent;

			const auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();
			out << YAML::Key << "Group" << YAML::Value << cc2d.Group;
			out << YAML::Key << "Offset" << YAML::Value << cc2d.Offset;
			out << YAML::Key << "Radius" << YAML::Value << cc2d.Radius;

			out << YAML::Key << "Density" << YAML::Value << cc2d.Density;
			out << YAML::Key << "Friction" << YAML::Value << cc2d.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << cc2d.Restitution;
			out << YAML::Key << "RestitutionThreshold" << YAML::Value << cc2d.RestitutionThreshold;

			out << YAML::EndMap; // !CircleCollider2DComponent
		}

		if (entity.HasComponent<NativeScriptComponent>())
		{
			auto& nsc = entity.GetComponent<NativeScriptComponent>();
		}
		out << YAML::EndMap; // !Entity
	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Untitled";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

		m_Scene->m_Registry.each([&](auto entityID)
		{
			const Entity entity = {entityID, m_Scene.get()};
			if (!entity)
				return;

			SerializeEntity(out, entity);
		});

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();

		OGN_CORE_TRACE("Scene Serialized in {0}", filepath);
	}

	void SceneSerializer::Serialize(const std::filesystem::path& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << filepath.filename().string();
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		m_Scene->m_Registry.each([&](auto entityID)
		{
			Entity entity = {entityID, m_Scene.get()};
			if (!entity)
				return;

			SerializeEntity(out, entity);
		});

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath.string());
		fout << out.c_str();

		OGN_CORE_INFO("Scene Serialized in {0}", filepath.string());
	}

	void SceneSerializer::SerializeRuntime(const std::string& filepath)
	{
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		std::ifstream stream(filepath);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Scene"])
			return false;

		auto sceneName = data["Scene"].as<std::string>();
		OGN_CORE_TRACE("Deserializing scene '{0}'", sceneName);

		if (YAML::Node entities = data["Entities"])
		{
			for (auto entity : entities)
			{
				auto uuid = entity["Entity"].as<uint64_t>();

				std::string name;
				if (auto tagComponent = entity["TagComponent"])
					name = tagComponent["Tag"].as<std::string>();

				Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

				if (YAML::Node transformComponent = entity["TransformComponent"])
				{
					auto& tc = deserializedEntity.GetComponent<TransformComponent>();
					tc.Translation = transformComponent["Translation"].as<glm::vec3>();
					tc.Rotation = transformComponent["Rotation"].as<glm::vec3>();
					tc.Scale = transformComponent["Scale"].as<glm::vec3>();
				}

				if (YAML::Node audioListnerComponent = entity["AudioListenerComponent"])
				{
					auto& al = deserializedEntity.AddComponent<AudioListenerComponent>();
					al.Enable = audioListnerComponent["Enable"].as<bool>();
				}

				if (YAML::Node animationComponent = entity["AnimationComponent"])
				{
					auto& ac = deserializedEntity.AddComponent<AnimationComponent>();

					if (auto states = animationComponent["States"])
					{
						// Get all states
						for (auto state : states)
						{
							Animation animation;

							ac.State.AddState(state["Name"].as<std::string>());
							
							// Retrieve all frames from the state
							for (auto frames : state["Frames"])
							{
								// Get the frames's filepath
								std::string framePath = Project::GetAssetFileSystemPath(frames["Path"].as<std::string>()).generic_string();
								const std::shared_ptr<Texture2D> texture = Texture2D::Create(framePath);
								animation.AddFrame(texture, 0.23f);
							}

							// Add the animation after frames added
							ac.State.AddAnimation(animation);
							ac.State.SetLooping(state["Looping"].as<bool>());
						}
					}

					ac.State.SetActiveState(animationComponent["DefaultState"].as<std::string>());
				}

				if (YAML::Node audioComponent = entity["AudioComponent"])
				{
					auto& ac = deserializedEntity.AddComponent<AudioComponent>();
					ac.Audio = Audio::Create();

					ac.Name = audioComponent["Name"].as<std::string>();
					ac.Volume = audioComponent["Volume"].as<float>();
					ac.Pitch = audioComponent["Pitch"].as<float>();
					ac.MinDistance = audioComponent["MinDistance"].as<float>();
					ac.MaxDistance = audioComponent["MaxDistance"].as<float>();
					ac.Looping = audioComponent["Looping"].as<bool>();
					ac.Spatial = audioComponent["Spatial"].as<bool>();
					ac.PlayAtStart = audioComponent["PlayAtStart"].as<bool>();

					std::string& filepath = audioComponent["Filepath"].as<std::string>();

					if (!filepath.empty())
					{
						AudioConfig config;
						config.Name = ac.Name;
						config.Looping = ac.Looping;
						config.MinDistance = ac.MinDistance;
						config.MaxDistance = ac.MaxDistance;
						config.Spatial = ac.Spatial;
						config.Filepath = Project::GetAssetFileSystemPath(filepath).generic_string();
						ac.Audio->LoadSource(config);
					}
				}

				if (YAML::Node cameraComponent = entity["CameraComponent"])
				{
					auto& cc = deserializedEntity.AddComponent<CameraComponent>();

					auto& cameraProps = cameraComponent["Camera"];
					cc.Camera.SetProjectionType(
						static_cast<SceneCamera::ProjectionType>(cameraProps["ProjectionType"].as<int>()));

					cc.Camera.SetPerspectiveFov(cameraProps["PerspectiveFOV"].as<float>());
					cc.Camera.SetPerspectiveNearClip(cameraProps["PerspectiveNear"].as<float>());
					cc.Camera.SetPerspectiveFarClip(cameraProps["PerspectiveFar"].as<float>());

					cc.Camera.SetOrthographicSize(cameraProps["OrthographicSize"].as<float>());
					cc.Camera.SetOrthographicNearClip(cameraProps["OrthographicNear"].as<float>());
					cc.Camera.SetOrthographicFarClip(cameraProps["OrthographicFar"].as<float>());

					cc.Primary = cameraComponent["Primary"].as<bool>();
					cc.FixedAspectRatio = cameraComponent["FixedAspectRatio"].as<bool>();
				}

				if (YAML::Node spriteRendererComponent = entity["SpriteRendererComponent"])
				{
					auto& src = deserializedEntity.AddComponent<SpriteRendererComponent>();
					src.Color = spriteRendererComponent["Color"].as<glm::vec4>();
					if (spriteRendererComponent["TexturePath"])
						src.Texture = Texture2D::Create(spriteRendererComponent["TexturePath"].as<std::string>());
				}

				if (YAML::Node particleComponent = entity["ParticleComponent"])
				{
					auto& pc = deserializedEntity.AddComponent<ParticleComponent>();
					pc.Velocity = particleComponent["Velocity"].as<glm::vec3>();
					pc.VelocityVariation = particleComponent["VelocityVariation"].as<glm::vec3>();
					pc.Rotation = particleComponent["Rotation"].as<glm::vec3>();
					pc.ColorBegin = particleComponent["ColorBegin"].as<glm::vec4>();
					pc.ColorEnd = particleComponent["ColorEnd"].as<glm::vec4>();
					pc.SizeBegin = particleComponent["SizeBegin"].as<float>();
					pc.SizeEnd = particleComponent["SizeEnd"].as<float>();
					pc.SizeVariation = particleComponent["SizeVariation"].as<float>();
					pc.ZAxis = particleComponent["ZAxis"].as<float>();
					pc.LifeTime = particleComponent["LifeTime"].as<float>();
				}

				if (YAML::Node spriteRenderer2DComponent = entity["SpriteRenderer2DComponent"])
				{
					auto& src = deserializedEntity.AddComponent<SpriteRenderer2DComponent>();
					src.Color = spriteRenderer2DComponent["Color"].as<glm::vec4>();
					if (spriteRenderer2DComponent["TexturePath"])
					{
						auto texturePath = spriteRenderer2DComponent["TexturePath"].as<std::string>();
						auto path = Project::GetAssetFileSystemPath(texturePath);
						src.Texture = Texture2D::Create(path.string());
					}

					src.TillingFactor = spriteRenderer2DComponent["TillingFactor"].as<glm::vec2>();
				}

				if (YAML::Node lightComponent = entity["LightComponent"])
				{
					auto& light = deserializedEntity.AddComponent<LightComponent>().Light;
					light = Lighting::Create(Utils::LightTypeStringToType(lightComponent["Type"].as<std::string>()));

					light->Color = lightComponent["Color"].as<glm::vec3>();

					switch (light->Type)
					{
					case LightingType::Spot:
					{
						light->InnerConeAngle = lightComponent["InnerConeAngle"].as<float>();
						light->OuterConeAngle = lightComponent["OuterConeAngle"].as<float>();
						light->Exponent = lightComponent["Exponent"].as<float>();
						break;
					}
					case LightingType::Point:
					{
						light->Ambient = lightComponent["Ambient"].as<float>();
						light->Specular = lightComponent["Specular"].as<float>();
						break;
					}
					case LightingType::Directional:
					{
						light->Ambient = lightComponent["Ambient"].as<float>();
						light->Diffuse = lightComponent["Diffuse"].as<float>();
						light->Specular = lightComponent["Specular"].as<float>();
						light->Near = lightComponent["Near"].as<float>();
						light->Far = lightComponent["Far"].as<float>();
						light->Size = lightComponent["Size"].as<float>();

						break;
					}

					default:
						break;
					}
				}

				if (YAML::Node circleRendererComponent = entity["CircleRendererComponent"])
				{
					auto& src = deserializedEntity.AddComponent<CircleRendererComponent>();
					src.Color = circleRendererComponent["Color"].as<glm::vec4>();
					src.Fade = circleRendererComponent["Fade"].as<float>();
					src.Thickness = circleRendererComponent["Thickness"].as<float>();
				}

				if (YAML::Node rigidbody2DComponent = entity["Rigidbody2DComponent"])
				{
					auto& rb2d = deserializedEntity.AddComponent<Rigidbody2DComponent>();
					rb2d.Type = Rigidbody2DBodyTypeFromString(rigidbody2DComponent["BodyType"].as<std::string>());
					rb2d.Mass = rigidbody2DComponent["Mass"].as<float>();
					rb2d.LinearDamping = rigidbody2DComponent["LinearDamping"].as<float>();
					rb2d.AngularDamping = rigidbody2DComponent["AngularDamping"].as<float>();
					rb2d.RotationalInertia = rigidbody2DComponent["RotationalInertia"].as<float>();
					rb2d.GravityScale = rigidbody2DComponent["GravityScale"].as<float>();
					rb2d.MassCenter = rigidbody2DComponent["MassCenter"].as<glm::vec2>();
					rb2d.FreezePositionX = rigidbody2DComponent["FreezePositionX"].as<bool>();
					rb2d.FreezePositionY = rigidbody2DComponent["FreezePositionY"].as<bool>();
					rb2d.AllowSleeping = rigidbody2DComponent["AllowSleeping"].as<bool>();
					rb2d.Awake = rigidbody2DComponent["Awake"].as<bool>();
					rb2d.Bullet = rigidbody2DComponent["Bullet"].as<bool>();
					rb2d.Enabled = rigidbody2DComponent["Enabled"].as<bool>();
					rb2d.FixedRotation = rigidbody2DComponent["FixedRotation"].as<bool>();
				}

				if (YAML::Node boxCollider2DComponent = entity["BoxCollider2DComponent"])
				{
					auto& bc2d = deserializedEntity.AddComponent<BoxCollider2DComponent>();
					bc2d.Group = boxCollider2DComponent["Group"].as<int>();
					bc2d.Offset = boxCollider2DComponent["Offset"].as<glm::vec2>();
					bc2d.Size = boxCollider2DComponent["Size"].as<glm::vec2>();
					bc2d.Density = boxCollider2DComponent["Density"].as<float>();
					bc2d.Friction = boxCollider2DComponent["Friction"].as<float>();
					bc2d.Restitution = boxCollider2DComponent["Restitution"].as<float>();
					bc2d.RestitutionThreshold = boxCollider2DComponent["RestitutionThreshold"].as<float>();
				}

				if (YAML::Node circleCollider2DComponent = entity["CircleCollider2DComponent"])
				{
					auto& cc2d = deserializedEntity.AddComponent<CircleCollider2DComponent>();
					cc2d.Group = circleCollider2DComponent["Group"].as<int>();
					cc2d.Offset = circleCollider2DComponent["Offset"].as<glm::vec2>();
					cc2d.Radius = circleCollider2DComponent["Radius"].as<float>();
					cc2d.Density = circleCollider2DComponent["Density"].as<float>();
					cc2d.Friction = circleCollider2DComponent["Friction"].as<float>();
					cc2d.Restitution = circleCollider2DComponent["Restitution"].as<float>();
					cc2d.RestitutionThreshold = circleCollider2DComponent["RestitutionThreshold"].as<float>();
				}

				if (YAML::Node staticMeshComponent = entity["StaticMeshComponent"])
				{
					auto& sMesh = deserializedEntity.AddComponent<StaticMeshComponent>();
					auto modelFilepath = staticMeshComponent["ModelPath"].as<std::string>();
					auto shaderFilepath = staticMeshComponent["ShaderPath"].as<std::string>();

					if (!modelFilepath.empty() && !shaderFilepath.empty())
					{
						auto& modelPath = Project::GetAssetFileSystemPath(modelFilepath);

						// Prepare The Materials
						std::shared_ptr<Shader> shader = Shader::Create(shaderFilepath);

						sMesh.Material = Material::Create(staticMeshComponent["MaterialName"].as<std::string>());
						sMesh.Material->LoadShader(shader);
						sMesh.Material->Color = staticMeshComponent["Color"].as<glm::vec4>();
						sMesh.Material->Shininess = staticMeshComponent["Shininess"].as<float>();
						sMesh.Material->Bias = staticMeshComponent["Bias"].as<float>();

						std::string& textureFilepath = staticMeshComponent["TextureFilepath"].as<std::string>();
						if (!textureFilepath.empty())
						{
							std::string& path = Project::GetAssetFileSystemPath(textureFilepath).generic_string();
							sMesh.Material->LoadTextureFromFile(path);
							sMesh.Material->TilingFactor = staticMeshComponent["TilingFactor"].as<glm::vec2>();
						}
						else
						{
							sMesh.Material->TilingFactor = glm::vec2(1.0f);
						}

						// Create The Model After
						sMesh.Model = Model::Create(modelPath.string(), sMesh.Material);
					}
				}

				if (YAML::Node boxColliderComponent = entity["BoxColliderComponent"])
				{
					auto& boxCollider = deserializedEntity.AddComponent<BoxColliderComponent>();
					boxCollider.Size = boxColliderComponent["Size"].as<glm::vec3>();
					boxCollider.Offset = boxColliderComponent["Offset"].as<glm::vec3>();
					boxCollider.Restitution = boxColliderComponent["Restitution"].as<float>();
					boxCollider.StaticFriction = boxColliderComponent["StaticFriction"].as<float>();
					boxCollider.DynamicFriction = boxColliderComponent["DynamicFriction"].as<float>();
				}

				if (YAML::Node sphereColliderComponent = entity["SpherColliderComponent"])
				{
					auto& boxCollider = deserializedEntity.AddComponent<SphereColliderComponent>();
					boxCollider.Radius = sphereColliderComponent["Radius"].as<float>();
					boxCollider.Offset = sphereColliderComponent["Offset"].as<glm::vec3>();
					boxCollider.Restitution = sphereColliderComponent["Restitution"].as<float>();
					boxCollider.StaticFriction = sphereColliderComponent["StaticFriction"].as<float>();
					boxCollider.DynamicFriction = sphereColliderComponent["DynamicFriction"].as<float>();
				}

				if (YAML::Node rigidbodyComponent = entity["RigidbodyComponent"])
				{
					auto& rigidbody = deserializedEntity.AddComponent<RigidbodyComponent>();

					rigidbody.Mass = rigidbodyComponent["Mass"].as<float>();
					rigidbody.CenterMassPosition = rigidbodyComponent["CenterMassPosition"].as<glm::vec3>();
					rigidbody.UseGravity = rigidbodyComponent["UseGravity"].as<bool>();
					rigidbody.RotateX = rigidbodyComponent["RotateX"].as<bool>();
					rigidbody.RotateY = rigidbodyComponent["RotateY"].as<bool>();
					rigidbody.RotateZ = rigidbodyComponent["RotateZ"].as<bool>();
					rigidbody.MoveX = rigidbodyComponent["MoveX"].as<bool>();
					rigidbody.MoveY = rigidbodyComponent["MoveY"].as<bool>();
					rigidbody.MoveZ = rigidbodyComponent["MoveZ"].as<bool>();
					rigidbody.Kinematic = rigidbodyComponent["Kinematic"].as<bool>();
					rigidbody.RetainAcceleration = rigidbodyComponent["RetainAcceleration"].as<bool>();

				}

				if (YAML::Node textComponent = entity["TextComponent"])
				{
					auto& text = deserializedEntity.AddComponent<TextComponent>();
					auto filepath = textComponent["FontFilepath"].as<std::string>();

					if (filepath.empty() || filepath == Font::GetDefault()->GetFilepath())
						text.FontAsset = Font::GetDefault();

					else if (!filepath.empty() && !text.FontAsset)
					{
						auto path = Project::GetAssetFileSystemPath(filepath);
						text.FontAsset = std::make_shared<Font>(path);
					}

					text.TextString = textComponent["TextString"].as<std::string>();
					text.Color = textComponent["Color"].as<glm::vec4>();
					text.Kerning = textComponent["Kerning"].as<float>();
					text.LineSpacing = textComponent["LineSpacing"].as<float>();
				}

				if (YAML::Node scriptComponent = entity["ScriptComponent"])
				{
					auto& sc = deserializedEntity.AddComponent<ScriptComponent>();
					OGN_CORE_ASSERT(deserializedEntity, "Entity is invalid");

					sc.ClassName = scriptComponent["ClassName"].as<std::string>();

					if (auto scriptFields = scriptComponent["ScriptFields"])
					{
						std::shared_ptr<ScriptClass> entityClass = ScriptEngine::GetEntityClass(sc.ClassName);
						OGN_CORE_ASSERT(entityClass, "Entity Class is Invalid");

						const auto& fields = entityClass->GetFields();
						auto& entityFields = ScriptEngine::GetScriptFieldMap(deserializedEntity);

						for (auto scriptField : scriptFields)
						{
							auto name = scriptField["Name"].as<std::string>();
							auto typeString = scriptField["Type"].as<std::string>();
							ScriptFieldType type = Utils::ScriptFieldTypeFromString(typeString);

							ScriptFieldInstance& fieldInstance = entityFields[name];

							// for Log in editor
							OGN_CORE_ASSERT(fields.find(name) != fields.end(), "Script Fields Not Found");
							if (fields.find(name) == fields.end())
								continue;

							fieldInstance.Field = fields.at(name);

							switch (type)
							{
							READ_FIELD_TYPE(Float, float);
							READ_FIELD_TYPE(Double, double);
							READ_FIELD_TYPE(Bool, bool);
							READ_FIELD_TYPE(Char, char);
							READ_FIELD_TYPE(Byte, int8_t);
							READ_FIELD_TYPE(Short, int16_t);
							READ_FIELD_TYPE(Int, int32_t);
							READ_FIELD_TYPE(Long, int64_t);
							READ_FIELD_TYPE(UByte, uint8_t);
							READ_FIELD_TYPE(UShort, uint16_t);
							READ_FIELD_TYPE(UInt, uint32_t);
							READ_FIELD_TYPE(ULong, uint64_t);
							READ_FIELD_TYPE(Vector2, glm::vec2);
							READ_FIELD_TYPE(Vector3, glm::vec3);
							READ_FIELD_TYPE(Vector4, glm::vec4);
							READ_FIELD_TYPE(Entity, UUID);
							}
						}
					}
				}
			}
		}

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const std::string& filepath)
	{
		OGN_CORE_TRACE(false);
		return false;
	}
}
