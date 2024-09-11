// Copyright (c) 2022-present Evangelion Manuhutu | ORigin Engine

#ifndef MESH_VERTEX_DATA_H
#define MESH_VERTEX_DATA_H

#include "Origin/Asset/Asset.h"
#include "Origin/Animation/ModelAnimation.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

namespace origin
{
    class OGN_API VertexArray;
    class OGN_API VertexBuffer;
    class OGN_API Texture2D;

    struct OGN_API StaticMeshVertexData
    {
        glm::vec3 Position;
        glm::vec3 Normals;
        glm::vec3 Color;
        glm::vec2 UV;
        glm::vec2 TilingFactor;
        float AlbedoIndex = 0.0f;
        float SpecularIndex = 0.0f;
    };

    class OGN_API StaticMeshData : public Asset
    {
    public:
        std::vector<StaticMeshVertexData> vertices;
        std::vector<uint32_t> indices;
        std::shared_ptr<VertexArray> vertexArray;
        std::shared_ptr<VertexBuffer> vertexBuffer;

        static AssetType GetStaticType() { return AssetType::StaticMesh; }
        virtual AssetType GetType() const { return GetStaticType(); }
    };

    // ================================
    // Dynamic Mesh

#define MAX_BONE_INFLUENCE 4
    struct OGN_API MeshVertexData
    {
        glm::vec3 Position;
        glm::vec3 Normals;
        glm::vec2 UV;
        int IDs[MAX_BONE_INFLUENCE];
        float Weights[MAX_BONE_INFLUENCE];
    };

    struct OGN_API BoneInfo
    {
        int ID;
        glm::mat4 OffsetMatrix;
    };

    class OGN_API MeshData : public Asset
    {
    public:
        std::vector<MeshVertexData> vertices;
        std::vector<uint32_t> indices;
        std::shared_ptr<VertexArray> vertexArray;
        std::shared_ptr<VertexBuffer> vertexBuffer;

        std::vector<ModelAnimation> animations;
        std::map<std::string, BoneInfo> boneInfoMap;
        int boneCount = 0;

        std::shared_ptr<Texture2D> DiffuseTexture;

        static AssetType GetStaticType() { return AssetType::Mesh; }
        virtual AssetType GetType() const { return GetStaticType(); }
    };
}

#endif