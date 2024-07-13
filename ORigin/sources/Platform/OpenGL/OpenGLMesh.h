// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine
#pragma once
#include "Origin/Renderer/Mesh.h"
#include "Origin/Renderer/VertexArray.h"
#include "Origin/Renderer/Buffer.h"

#include <vector>

namespace origin
{
	class UniformBuffer;
	class OpenGLMesh : public Mesh
	{
	public:
		OpenGLMesh() = default;

		OpenGLMesh(const std::vector<MeshVertex>& vertices, const std::vector<uint32_t>& indices);
		~OpenGLMesh() override;

		void Draw(const glm::mat4 &transform, int entityId = -1) override;
	};
}