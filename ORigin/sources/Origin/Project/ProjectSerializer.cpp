// Copyright (c) Evangelion Manuhutu | ORigin Engine

#include "pch.h"
#include "ProjectSerializer.h"

#include <stdint.h>
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace origin
{
	ProjectSerializer::ProjectSerializer(std::shared_ptr<Project> project)
		: m_Project(project)
	{
	}

	bool ProjectSerializer::Serialize(const std::filesystem::path& filepath)
	{
		const auto& config = m_Project->GetConfig();

		YAML::Emitter out;
		out << YAML::BeginMap;
		{
			out << YAML::Key << "Project" << YAML::Value;
			out << YAML::BeginMap;
			{
				out << YAML::Key << "Name" << YAML::Value << config.Name;
				out << YAML::Key << "StartScene" << YAML::Value << (uint64_t)config.StartScene;
				out << YAML::Key << "AssetDirectory" << YAML::Value << config.AssetDirectory.string();
				out << YAML::Key << "AssetRegistry" << YAML::Value << config.AssetRegistry.string();
				out << YAML::Key << "ScriptModulePath" << YAML::Value << config.ScriptModulePath.string();
				out << YAML::EndMap;
			}
			out << YAML::EndMap;
		}

		std::ofstream fout(filepath.string());
		fout << out.c_str();

		// Serialized the updated AssetRegistry
		m_Project->GetEditorAssetManager()->SerializeAssetRegistry();

		return true;
	}

	bool ProjectSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		auto& config = m_Project->GetConfig();

		YAML::Node data = YAML::LoadFile(filepath.string());

		YAML::Node projectNode = data["Project"];
		if (!projectNode)
			return false;

		config.Name = projectNode["Name"].as<std::string>();
		config.StartScene = projectNode["StartScene"].as<uint64_t>();
		config.AssetDirectory = projectNode["AssetDirectory"].as<std::string>();
		config.AssetRegistry = projectNode["AssetRegistry"].as<std::string>();
		config.ScriptModulePath = projectNode["ScriptModulePath"].as<std::string>();

		OGN_CORE_INFO("ProjectSerializer::Deserializing {}", filepath.string());
		OGN_CORE_INFO("Name				: {}", config.Name);
		OGN_CORE_INFO("Start Scene: {}", config.StartScene);
		OGN_CORE_INFO("Asset Reg	: {}", config.AssetRegistry.string());

		return true;
	}

}