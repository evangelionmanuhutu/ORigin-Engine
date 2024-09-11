// Copyright (c) 2022-present Evangelion Manuhutu | ORigin Engine

#ifndef ASSET_IMPORTER_H
#define ASSET_IMPORTER_H

#include "AssetMetadata.h"
#include "Origin/Audio/AudioSource.h"
#include "Origin/Scene/SpriteSheet.h"
#include "Origin/Renderer/Texture.h"

#include <queue>
#include <future>

namespace origin
{
    class OGN_API Scene;
    using AssetImportFunction = std::function<Ref<Asset>(AssetHandle, const AssetMetadata &)>;

    class OGN_API AssetTaskBase
    {
    public:
        virtual ~AssetTaskBase() = default;
        virtual bool Execute() = 0;
    };

    template<typename FutureT>
    class OGN_API AssetTask : public AssetTaskBase
    {
    public:
        Ref<Asset> *Value;
        std::future<FutureT> Future;

        template<typename Func, typename... Args>
        AssetTask(Ref<Asset> *asset, Func &&func, Args&&... args)
            : Value(asset)
        {
            Future = std::async(std::launch::async, std::forward<Func>(func), std::forward<Args>(args)...);
        }

        // move constructor to allow transfer of ownership of Future
        AssetTask(AssetTask &&other) noexcept
            : Value(std::move(other.Value)), Future(std::move(other.Future))
        {}

        // this should be executed in main thread
        virtual bool Execute() override
        {
            if (Future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
            {
                void *data = Future.get();
                if (static_cast<FontData *>(data))
                {
                    *Value = Font::Create((FontData *)data);
                }
                return true;
            }
            return false;
        }

        // move assignment operator
        AssetTask &operator=(AssetTask &&other) noexcept
        {
            if (this != &other)
            {
                Value = std::move(other.Value);
                Future = std::move(other.Future);
            }
            return *this;
        }

        // disable copy constructor and assignment operator
        //AssetTask(const AssetTask &) = delete;
        //AssetTask &operator=(const AssetTask &) = delete;
    };

    struct OGN_API AssetTaskWorker
    {
        template<typename T>
        void AddTask(AssetTask<T> &task)
        {
            TaskQueue.push(CreateScope<AssetTask<T>>(std::move(task)));
        }

        // update this on main thread
        void Update(Timestep ts);

        std::queue<Scope<AssetTaskBase>> TaskQueue;
        f32 Timer = 0.0f;
    };

    class OGN_API AssetImporter
    {
    public:
        static Ref<Asset> ImportAsset(AssetHandle handle, const AssetMetadata& metadata);
        static void SyncToMainThread(Timestep ts);
    };

    class OGN_API FontImporter
    {
    public:
        static Ref<Font> Import(AssetHandle handle, AssetMetadata metadata);
        static void LoadAsync(Ref<Asset> *font, const std::filesystem::path &filepath);

    private:
        friend class AssetImporter;
    };

    class OGN_API AudioImporter
    {
    public:
        static Ref<AudioSource> Import(AssetHandle handle, AssetMetadata metadata);
        static Ref<AudioSource> LoadAudioSource(const std::filesystem::path &filepath);
        static Ref<AudioSource> LoadStreamingSource(const std::filesystem::path &filepath);
    };

    class OGN_API SceneImporter
    {
    public:
        static Ref<Scene> Import(AssetHandle handle, const AssetMetadata &metadata);
        static Ref<Scene> LoadScene(const std::filesystem::path &filepath);
        static AssetHandle OpenScene(const std::filesystem::path &filepath);
        static void SaveScene(Ref<Scene> scene, const std::filesystem::path &path);
    };

    class OGN_API TextureImporter
    {
    public:
        static Ref<Texture2D> ImportTexture2D(AssetHandle handle, const AssetMetadata &metadata);
        static Ref<Texture2D> LoadTexture2D(const std::filesystem::path &path);
    };

    class OGN_API ModelImporter
    {
    public:
        static Ref<StaticMeshData> ImportStaticMesh(AssetHandle handle, const AssetMetadata &metadata);
        static Ref<StaticMeshData> LoadStaticMesh(const std::filesystem::path &filepath);

        static Ref<MeshData> ImportMesh(AssetHandle handle, const AssetMetadata &metadata);
        static Ref<MeshData> LoadMesh(const std::filesystem::path &filepath);
    };

    class OGN_API SpriteSheetImporter
    {
    public:
        static Ref<SpriteSheet> Import(AssetHandle handle, const AssetMetadata &metadata);
        static Ref<SpriteSheet> Load(const std::filesystem::path &filepath);
    };

    class OGN_API MaterialImporter
    {
    public:
        static Ref<Material> Import(AssetHandle handle, const AssetMetadata &metadata);
        static Ref<Material> Load(const std::filesystem::path &filepath);
    };
}

#endif