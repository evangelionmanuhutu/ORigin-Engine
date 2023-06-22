// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#include "pch.h"

#include "SceneSerializer.h"
#include "Origin\Scripting\ScriptEngine.h"
#include "Origin\Project\Project.h"
#include "Origin\Renderer\Model.h"
#include "Origin\Audio\Audio.h"

#include "Entity.h"
#include "Component.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace YAML
{
	template<>
	struct convert<origin::UUID>
	{
		static Node encode(const origin::UUID uuid)
		{
			Node node;
			node.push_back((uint64_t)uuid);
			return node;
		}

		static bool decode(const Node& node, origin::UUID uuid)
		{
			uuid = node.as<uint64_t>();
			return true;
		}
	};

	template<>
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

	template<>
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

	template<>
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
namespace origin {

#define WRITE_FIELD_TYPE(FieldType, Type)\
				case ScriptFieldType::FieldType:\
				{\
					out << scriptField.GetValue<Type>();\
					break;\
				}\

#define	READ_FIELD_TYPE(FieldType, Type)\
				case ScriptFieldType::FieldType:\
				{\
					Type data = scriptField["Data"].as<Type>();\
					fieldInstance.SetValue(data);\
					break;\
				}\

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
			case Rigidbody2DComponent::BodyType::Static:    return "Static";
			case Rigidbody2DComponent::BodyType::Dynamic:   return "Dynamic";
			case Rigidbody2DComponent::BodyType::Kinematic: return "Kinematic";
		}

		OGN_CORE_ASSERT(false, "Unknown body type");
		return {};
	}

	static Rigidbody2DComponent::BodyType RigidBody2DBodyTypeFromString(const std::string& bodyTypeString)
	{
		if (bodyTypeString == "Static")    return Rigidbody2DComponent::BodyType::Static;
		if (bodyTypeString == "Dynamic")   return Rigidbody2DComponent::BodyType::Dynamic;
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

		if (entity.HasComponent<AudioComponent>())
		{
			out << YAML::Key << "AudioComponent";
			out << YAML::BeginMap; // AudioComponent

			const auto& ac = entity.GetComponent<AudioComponent>();
			out << YAML::Key << "Name" << YAML::Value << ac.Name;
			out << YAML::Key << "Filepath" << YAML::Value << ac.Audio->GetFilepath();
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
			out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
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

		if (entity.HasComponent<StaticMeshComponent>())
		{
			out << YAML::Key << "StaticMeshComponent";
			out << YAML::BeginMap; // StaticMeshComponent

			const auto& sMesh = entity.GetComponent<StaticMeshComponent>();
			out << YAML::Key << "ModelPath" << YAML::Value << sMesh.ModelPath;
			out << YAML::Key << "ShaderPath" << YAML::Value << sMesh.ShaderPath;
			out << YAML::Key << "Color" << YAML::Value << sMesh.Color;

			out << YAML::EndMap; // !StaticMeshComponent
		}

		if (entity.HasComponent<TextComponent>())
		{
			out << YAML::Key << "TextComponent";
			out << YAML::BeginMap; // TextComponent
			auto& textComponent = entity.GetComponent<TextComponent>();
			out << YAML::Key << "TextString" << YAML::Value << textComponent.TextString;
			out << YAML::Key << "FontFilepath" << YAML::Value << textComponent.FontAsset->GetFilepath();
			out << YAML::Key << "Color" << YAML::Value << textComponent.Color;
			out << YAML::Key << "Kerning" << YAML::Value << textComponent.Kerning;
			out << YAML::Key << "LineSpacing" << YAML::Value << textComponent.LineSpacing;

			out << YAML::EndMap; // !TextComponent
		}

		if (entity.HasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::BeginMap; // SpriteRendererComponent

			const auto& src = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << src.Color;
			if (src.Texture)
				out << YAML::Key << "TexturePath" << YAML::Value << src.Texture->GetFilepath();

			out << YAML::EndMap; // !SpriteRendererComponent
		}

		if (entity.HasComponent<SpriteRenderer2DComponent>())
		{
			out << YAML::Key << "SpriteRenderer2DComponent";
			out << YAML::BeginMap; // SpriteRenderer2DComponent

			const auto& src = entity.GetComponent<SpriteRenderer2DComponent>();
			out << YAML::Key << "Color" << YAML::Value << src.Color;
			if(src.Texture)
				out << YAML::Key << "TexturePath" << YAML::Value << src.Texture->GetFilepath();
			out << YAML::Key << "TillingFactor" << YAML::Value << src.TillingFactor;

			out << YAML::EndMap; // !SpriteRenderer2DComponent
		}

		if (entity.HasComponent<DirectionalLightComponent>())
		{
			out << YAML::Key << "DirectionalLightComponent";
			out << YAML::BeginMap; // DirectionalLightComponent

			const auto& lc = entity.GetComponent<DirectionalLightComponent>();
			out << YAML::Key << "Ambient" << YAML::Value << lc.Ambient;
			out << YAML::Key << "Diffuse" << YAML::Value << lc.Diffuse;
			out << YAML::Key << "Specular" << YAML::Value << lc.Specular;

			out << YAML::EndMap; // !DirectionalLightComponent
		}

		if (entity.HasComponent<SpotLightComponent>())
		{
			out << YAML::Key << "SpotLightComponent";
			out << YAML::BeginMap; // SpotLightComponent

			const auto& lc = entity.GetComponent<SpotLightComponent>();
			out << YAML::Key << "Color" << YAML::Value << lc.Color;
			out << YAML::Key << "Ambient" << YAML::Value << lc.Ambient;
			out << YAML::Key << "Specular" << YAML::Value << lc.Specular;
			out << YAML::Key << "Outercone" << YAML::Value << lc.Outercone;
			out << YAML::Key << "Innercone" << YAML::Value << lc.Innercone;

			out << YAML::EndMap; // !SpotLightComponent
		}

		if (entity.HasComponent<PointLightComponent>())
		{
			out << YAML::Key << "PointLightComponent";
			out << YAML::BeginMap; // PointLightComponent

			const auto& lc = entity.GetComponent<PointLightComponent>();
			out << YAML::Key << "Color" << YAML::Value << lc.Color;
			out << YAML::Key << "Ambient" << YAML::Value << lc.Ambient;
			out << YAML::Key << "Specular" << YAML::Value << lc.Specular;

			out << YAML::EndMap; // !PointLightComponent
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
			out << YAML::Key << "FixedRotation" << YAML::Value << rb2d.FixedRotation;

			out << YAML::EndMap; // !Rigidbody2DComponent
		}

		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			out << YAML::Key << "BoxCollider2DComponent";
			out << YAML::BeginMap; // BoxCollider2DComponent;

			const auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();
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

			const auto& bc2d = entity.GetComponent<CircleCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << bc2d.Offset;
			out << YAML::Key << "Radius" << YAML::Value << bc2d.Radius;

			out << YAML::Key << "Density" << YAML::Value << bc2d.Density;
			out << YAML::Key << "Friction" << YAML::Value << bc2d.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << bc2d.Restitution;
			out << YAML::Key << "RestitutionThreshold" << YAML::Value << bc2d.RestitutionThreshold;

			out << YAML::EndMap; // !CircleCollider2DComponent
		}

		if (entity.HasComponent<NativeScriptComponent>())
		{
			auto& nsc = entity.GetComponent<NativeScriptComponent>();
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
				out << YAML::Key << "StoredFields" << YAML::Value;
				auto& entityFields = ScriptEngine::GetScriptFieldMap(entity);

				out << YAML::BeginSeq;
				for (const auto& [name, field] : fields)
				{
					if (entityFields.find(name) == entityFields.end())
						continue;

					out << YAML::BeginMap; // Fields
					out << YAML::Key << "Name" << YAML::Value << name;
					out << YAML::Key << "Type" << YAML::Value << Utils::ScriptFieldTypeToString(field.Type);
					out << YAML::Key << "Data" << YAML::Value;

					ScriptFieldInstance& scriptField = entityFields.at(name);
					switch (field.Type)
					{
						WRITE_FIELD_TYPE(Float,		float);
						WRITE_FIELD_TYPE(Double,	double);
						WRITE_FIELD_TYPE(Bool,		bool);
						WRITE_FIELD_TYPE(Char,		char);
						WRITE_FIELD_TYPE(Byte,		int8_t);
						WRITE_FIELD_TYPE(Short,		int16_t);
						WRITE_FIELD_TYPE(Int,			int32_t);
						WRITE_FIELD_TYPE(Long,		int64_t);
						WRITE_FIELD_TYPE(UByte,		uint8_t);
						WRITE_FIELD_TYPE(UShort,	uint16_t);
						WRITE_FIELD_TYPE(UInt,		uint32_t);
						WRITE_FIELD_TYPE(ULong,		uint64_t);
						WRITE_FIELD_TYPE(Vector2, glm::vec2);
						WRITE_FIELD_TYPE(Vector3, glm::vec3);
						WRITE_FIELD_TYPE(Vector4, glm::vec4);
						WRITE_FIELD_TYPE(Entity,	UUID);
					}

					out << YAML::EndMap; // !Fields
				}
				out << YAML::EndSeq;
			}

			out << YAML::EndMap; // !ScriptComponent;
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
			const Entity entity = { entityID, m_Scene.get() };
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
			Entity entity = { entityID, m_Scene.get() };
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

		std::string sceneName = data["Scene"].as<std::string>();
		OGN_CORE_TRACE("Deserializing scene '{0}'", sceneName);

		if (auto entities = data["Entities"])
		{
			for (auto entity : entities)
			{
				auto uuid = entity["Entity"].as<uint64_t>();

				std::string name;
				if (auto tagComponent = entity["TagComponent"])
					name = tagComponent["Tag"].as<std::string>();

				Entity deserializedEntity = m_Scene->CreateEntityWithUUID(uuid, name);

				if (auto transformComponent = entity["TransformComponent"])
				{
					auto& tc = deserializedEntity.GetComponent<TransformComponent>();
					tc.Translation = transformComponent["Translation"].as<glm::vec3>();
					tc.Rotation = transformComponent["Rotation"].as<glm::vec3>();
					tc.Scale = transformComponent["Scale"].as<glm::vec3>();
				}

				if (auto audioComponent = entity["AudioComponent"])
				{
					auto& ac = deserializedEntity.AddComponent<AudioComponent>();
					ac.Name = audioComponent["Name"].as<std::string>();
					ac.Volume = audioComponent["Volume"].as<float>();
					ac.Pitch = audioComponent["Pitch"].as<float>();
					ac.MinDistance = audioComponent["MinDistance"].as<float>();
					ac.MaxDistance = audioComponent["MaxDistance"].as<float>();
					ac.Looping = audioComponent["Looping"].as<bool>();
					ac.Spatial = audioComponent["Spatial"].as<bool>();
					ac.PlayAtStart = audioComponent["PlayAtStart"].as<bool>();

					if (audioComponent["Filepath"])
					{
						auto& filepath = audioComponent["Filepath"].as<std::string>();

						AudioConfig spec;
						spec.Name = ac.Name;
						spec.Looping = ac.Looping;
						spec.MinDistance = ac.MinDistance;
						spec.MaxDistance = ac.MaxDistance;
						spec.Spatial = ac.Spatial;
						spec.Filepath = Project::GetAssetFileSystemPath(filepath).string();

						ac.Audio = Audio::Create(spec);
					}
				}

				if (auto cameraComponent = entity["CameraComponent"])
				{
					auto& cc = deserializedEntity.AddComponent<CameraComponent>();

					auto& cameraProps = cameraComponent["Camera"];
					cc.Camera.SetProjectionType((SceneCamera::ProjectionType)cameraProps["ProjectionType"].as<int>());

					cc.Camera.SetPerspectiveFov(cameraProps["PerspectiveFOV"].as<float>());
					cc.Camera.SetPerspectiveNearClip(cameraProps["PerspectiveNear"].as<float>());
					cc.Camera.SetPerspectiveFarClip(cameraProps["PerspectiveFar"].as<float>());

					cc.Camera.SetOrthographicSize(cameraProps["OrthographicSize"].as<float>());
					cc.Camera.SetOrthographicNearClip(cameraProps["OrthographicNear"].as<float>());
					cc.Camera.SetOrthographicFarClip(cameraProps["OrthographicFar"].as<float>());

					cc.Primary = cameraComponent["Primary"].as<bool>();
					cc.FixedAspectRatio = cameraComponent["FixedAspectRatio"].as<bool>();
				}

				if (auto spriteRendererComponent = entity["SpriteRendererComponent"])
				{
					auto& src = deserializedEntity.AddComponent<SpriteRendererComponent>();
					src.Color = spriteRendererComponent["Color"].as<glm::vec4>();
					if (spriteRendererComponent["TexturePath"])
						src.Texture = Texture2D::Create(spriteRendererComponent["TexturePath"].as<std::string>());
				}

				if (auto spriteRenderer2DComponent = entity["SpriteRenderer2DComponent"])
				{
					auto& src = deserializedEntity.AddComponent<SpriteRenderer2DComponent>();
					src.Color = spriteRenderer2DComponent["Color"].as<glm::vec4>();
					if (spriteRenderer2DComponent["TexturePath"])
					{
						std::string texturePath = spriteRenderer2DComponent["TexturePath"].as<std::string>();
						auto path = Project::GetAssetFileSystemPath(texturePath);
						src.Texture = Texture2D::Create(path.string());
					}

					src.TillingFactor = spriteRenderer2DComponent["TillingFactor"].as<float>();
				}

				if (auto directionalLightComponent = entity["DirectionalLightComponent"])
				{
					auto& lc = deserializedEntity.AddComponent<DirectionalLightComponent>();
					lc.Ambient = directionalLightComponent["Ambient"].as<float>();
					lc.Diffuse = directionalLightComponent["Diffuse"].as<float>();
					lc.Specular = directionalLightComponent["Specular"].as<float>();
				}

				if (auto spotLightComponent = entity["SpotLightComponent"])
				{
					auto& lc = deserializedEntity.AddComponent<SpotLightComponent>();
					lc.Color = spotLightComponent["Color"].as<glm::vec3>();
					lc.Ambient = spotLightComponent["Ambient"].as<float>();
					lc.Specular = spotLightComponent["Specular"].as<float>();
					lc.Outercone = spotLightComponent["Outercone"].as<float>();
					lc.Innercone = spotLightComponent["Innercone"].as<float>();
				}

				if (auto pointLightComponent = entity["PointLightComponent"])
				{
					auto& lc = deserializedEntity.AddComponent<PointLightComponent>();
					lc.Color = pointLightComponent["Color"].as<glm::vec3>();
					lc.Ambient = pointLightComponent["Ambient"].as<float>();
					lc.Specular = pointLightComponent["Specular"].as<float>();
				}

				if (auto circleRendererComponent = entity["CircleRendererComponent"])
				{
					auto& src = deserializedEntity.AddComponent<CircleRendererComponent>();
					src.Color = circleRendererComponent["Color"].as<glm::vec4>();
					src.Fade = circleRendererComponent["Fade"].as<float>();
					src.Thickness = circleRendererComponent["Thickness"].as<float>();
				}

				if (auto rigidbody2DComponent = entity["Rigidbody2DComponent"])
				{
					auto& rb2d = deserializedEntity.AddComponent<Rigidbody2DComponent>();
					rb2d.Type = RigidBody2DBodyTypeFromString(rigidbody2DComponent["BodyType"].as<std::string>());
					rb2d.FixedRotation = rigidbody2DComponent["FixedRotation"].as<bool>();
				}

				if (auto staticMeshComponent = entity["StaticMeshComponent"])
				{
					auto& sMesh = deserializedEntity.AddComponent<StaticMeshComponent>();
					sMesh.ModelPath = staticMeshComponent["ModelPath"].as<std::string>();
					sMesh.ShaderPath = staticMeshComponent["ShaderPath"].as<std::string>();
					if (!sMesh.ModelPath.empty() && !sMesh.ShaderPath.empty())
					{
						std::shared_ptr<Shader> shader = Shader::Create(sMesh.ShaderPath);
						sMesh.Model = Model::Create(sMesh.ModelPath, shader);
					}
					sMesh.Color = staticMeshComponent["Color"].as<glm::vec4>();
				}

				if (auto textComponent = entity["TextComponent"])
				{
					auto& text = deserializedEntity.AddComponent<TextComponent>();
					const std::string filepath = textComponent["FontFilepath"].as<std::string>();
					
					if(filepath.empty() || filepath == Font::GetDefault()->GetFilepath())
						text.FontAsset = Font::GetDefault();
					
					else if(!filepath.empty() && !text.FontAsset)
					{
						auto path = Project::GetAssetFileSystemPath(filepath);
						text.FontAsset = std::make_shared<Font>(path);
					}
					
					text.TextString = textComponent["TextString"].as<std::string>();
					text.Color = textComponent["Color"].as<glm::vec4>();
					text.Kerning = textComponent["Kerning"].as<float>();
					text.LineSpacing = textComponent["LineSpacing"].as<float>();
				}

				if (auto boxCollider2DComponent = entity["BoxCollider2DComponent"])
				{
					auto& bc2d = deserializedEntity.AddComponent<BoxCollider2DComponent>();
					bc2d.Offset = boxCollider2DComponent["Offset"].as<glm::vec2>();
					bc2d.Size = boxCollider2DComponent["Size"].as<glm::vec2>();
					bc2d.Density = boxCollider2DComponent["Density"].as<float>();
					bc2d.Friction = boxCollider2DComponent["Friction"].as<float>();
					bc2d.Restitution = boxCollider2DComponent["Restitution"].as<float>();
					bc2d.RestitutionThreshold = boxCollider2DComponent["RestitutionThreshold"].as<float>();
				}

				if (auto circleCollider2DComponent = entity["CircleCollider2DComponent"])
				{
					auto& cc2d = deserializedEntity.AddComponent<CircleCollider2DComponent>();
					cc2d.Offset = circleCollider2DComponent["Offset"].as<glm::vec2>();
					cc2d.Radius = circleCollider2DComponent["Radius"].as<float>();
					cc2d.Density = circleCollider2DComponent["Density"].as<float>();
					cc2d.Friction = circleCollider2DComponent["Friction"].as<float>();
					cc2d.Restitution = circleCollider2DComponent["Restitution"].as<float>();
					cc2d.RestitutionThreshold = circleCollider2DComponent["RestitutionThreshold"].as<float>();
				}

				if (auto scriptComponent = entity["ScriptComponent"])
				{
					auto& sc = deserializedEntity.AddComponent<ScriptComponent>();
					sc.ClassName = scriptComponent["ClassName"].as<std::string>();

					if(auto scriptFields = scriptComponent["ScriptFields"])
					{
						std::shared_ptr<ScriptClass> entityClass = ScriptEngine::GetEntityClass(sc.ClassName);
						OGN_CORE_ASSERT(deserializedEntity, "Entity is invalid");

						const auto& fields = entityClass->GetFields();
						auto& entityFields = ScriptEngine::GetScriptFieldMap(deserializedEntity);

						for(auto scriptField : scriptFields)
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

							switch(type)
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

				if (auto nativeScriptComponent = entity["NativeScriptComponent"])
				{
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