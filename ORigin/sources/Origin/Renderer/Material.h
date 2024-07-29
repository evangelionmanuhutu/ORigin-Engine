// Copyright (c) Evangelion Manuhutu | ORigin Engine

#pragma once
#include "Shader.h"
#include "Texture.h"
#include "UniformBuffer.h"
#include "Origin/Asset/Asset.h"
#include "Origin/Profiler/Profiler.h"

#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <unordered_map>

namespace origin::Utils
{
	static std::vector<std::shared_ptr<Texture2D>> loadedTextures;

	static void LoadMatTextures(std::unordered_map<aiTextureType, std::shared_ptr<Texture2D>> *tex, const std::string &modelFilepath, aiMaterial *mat, aiTextureType type)
	{
		std::unordered_map<aiTextureType, std::shared_ptr<Texture2D>> textures;

		for (uint32_t i = 0; i < mat->GetTextureCount(type); i++)
		{
			OGN_PROFILER_SCOPE("Material::LoadTextures TextureCount");

			aiString str;

			mat->GetTexture(type, i, &str);
			bool skip = false;
			for (uint32_t j = 0; j < loadedTextures.size(); j++)
			{
				if (std::strcmp(loadedTextures[j]->GetName().c_str(), str.C_Str()) == 0)
				{
					tex->insert({ type, loadedTextures[j] });
					skip = true;
					break;
				}
			}

			if (!skip)
			{
				OGN_PROFILER_SCOPE("Material::LoadTextures TextureCount");

				auto textureDirectory = modelFilepath.substr(0, modelFilepath.find_last_of('/'));
				std::string textureName = std::string(str.C_Str());

				std::shared_ptr<Texture2D> newTexture = Texture2D::Create(textureDirectory + "/" + textureName);
				tex->insert({ type, newTexture });
				loadedTextures.push_back(newTexture);
			}
		}
		loadedTextures.clear();
	}
}

namespace origin
{
	struct MaterialBufferData
	{
		float Emission = 0.0f;
		float MetallicValue = 0.0f;
		float RoughnessValue = 0.0f;
		bool UseNormalMaps = false;
	};

	struct MaterialTexture
	{
		UUID Handle = 0;
		std::shared_ptr<Texture2D> Texture;
	};

	class Material : public Asset
	{
	public:
		Material(const std::string &name);
		Material(const std::shared_ptr<Shader> &shader);
		MaterialBufferData BufferData;

		void Bind();
		void Unbind();
		void AddShader(const std::shared_ptr<Shader> &shader);
		bool RefreshShader();

		void SetAlbedoMap(AssetHandle handle);
		void SetMetallicMap(AssetHandle handle);
		void SetName(const std::string &name) { m_Name = name; }

		const std::string &GetName() { return m_Name; }

		std::shared_ptr<Shader> m_Shader;
		static std::shared_ptr<Material> Create(const std::string &name);
		static std::shared_ptr<Material> Create(const std::shared_ptr<Shader> &shader);

		static AssetType GetStaticType() { return AssetType::Material; }
		virtual AssetType GetType() const { return GetStaticType(); }

		MaterialTexture Albedo;
		MaterialTexture Metallic;

		glm::vec4 Color = glm::vec4(1.0f);
		glm::vec2 TilingFactor = glm::vec2(1.0f);

	private:
		std::shared_ptr<UniformBuffer> m_UniformBuffer;
		friend class OpenGLModel;

		std::string m_Name = "Material";
	};
}
