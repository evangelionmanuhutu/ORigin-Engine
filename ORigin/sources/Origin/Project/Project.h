// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine
#pragma once

#include "Origin\Core\Assert.h"

#include <string>
#include <filesystem>

namespace origin
{
	struct ProjectConfig
	{
		std::string Name = "Untitled";

		std::filesystem::path StartScene;
		std::filesystem::path AssetDirectory;
		std::filesystem::path ScriptModulePath;
	};

	class Project
	{
	private:
		ProjectConfig m_Config;
		std::filesystem::path m_ProjectDirectory;

		inline static std::shared_ptr<Project> s_ActiveProject;

	public:
		static const std::filesystem::path& GetProjectDirectory()
		{
			OGN_CORE_ASSERT(s_ActiveProject)
			return s_ActiveProject->m_ProjectDirectory;
		}

		static std::filesystem::path GetAssetDirectory()
		{
			OGN_CORE_ASSERT(s_ActiveProject)
			return GetProjectDirectory() / s_ActiveProject->m_Config.AssetDirectory;
		}

		static std::filesystem::path GetAssetFileSystemPath(const std::filesystem::path& path)
		{
			OGN_CORE_ASSERT(s_ActiveProject)
			return GetAssetDirectory() / path;
		}
		
		static std::shared_ptr<Project> GetActive() { return s_ActiveProject; }
		ProjectConfig& GetConfig() { return m_Config; }

		static std::shared_ptr<Project> New();
		static std::shared_ptr<Project> Open();
		static std::shared_ptr<Project> Load(const std::filesystem::path& path);

		static bool SaveActive(const std::filesystem::path& path);
	};

}
