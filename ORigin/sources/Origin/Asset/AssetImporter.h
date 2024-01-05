// Copyright (c) Evangelion Manuhutu | ORigin Engine
#pragma once
#include "AssetMetadata.h"

#include "Origin\Audio\Audio.h"
#include "Origin\Scene\Scene.h"
#include "Origin\Renderer\Texture.h"
#include "Origin\Renderer\Model.h"

namespace origin {

	class AssetImporter
	{
	public:
		static std::shared_ptr<Asset> ImportAsset(AssetHandle handle, const AssetMetadata& metadata);
	};

	class AudioImporter
	{
	public:
		static std::shared_ptr<Audio> ImportAudio(AssetHandle handle, AssetMetadata metadata);
		static std::shared_ptr<Audio> LoadAudio(const std::filesystem::path filepath, AudioConfig config);
	};

	class SceneImporter
	{
	public:
		static std::shared_ptr<Scene> ImportScene(AssetHandle handle, const AssetMetadata& metadata);

		static std::shared_ptr<Scene> LoadScene(const std::filesystem::path& filepath);
		static AssetHandle OpenScene(const std::filesystem::path& filepath);
		static void SaveScene(std::shared_ptr<Scene> scene, const std::filesystem::path& path);
	};

	class TextureImporter
	{
	public:
		static std::shared_ptr<Texture2D> ImportTexture2D(AssetHandle handle, const AssetMetadata& metadata);
		static std::shared_ptr<Texture2D> LoadTexture2D(const std::filesystem::path& path);
		static std::shared_ptr<Texture2D> GetWhiteTexture();
	};

	class ModelImporter
	{
	public:
		static std::shared_ptr<Model> ImportModel(AssetHandle handle, const AssetMetadata& metadata);
		static std::shared_ptr<Model> LoadModel(const std::filesystem::path& path);
	};

}
