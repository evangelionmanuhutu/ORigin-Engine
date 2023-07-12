// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#pragma once
#include "Texture.h"

namespace origin {

	struct MSDFData;

	class Font
	{
	public:
		Font(const std::filesystem::path& filepath);
		~Font();

		const MSDFData* GetMSDFData() const { return m_Data; }
		const std::shared_ptr<Texture2D>& GetAtlasTexture() const { return m_AtlasTexture; }
		std::string GetFilepath() const { return m_Filepath; }
		
		bool IsLoaded() const { return m_Loaded; }
		static std::shared_ptr<Font> GetDefault();
		
	private:
		MSDFData* m_Data;
		std::shared_ptr<Texture2D> m_AtlasTexture;
		bool m_Loaded = false;
		std::string m_Filepath;
	};
}