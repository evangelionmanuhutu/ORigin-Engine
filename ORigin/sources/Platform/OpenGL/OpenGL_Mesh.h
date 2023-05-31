// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine
#pragma once
#include "Origin\Renderer\Mesh.h"
#include "Origin\Renderer\VertexArray.h"
#include "Origin\Renderer\Buffer.h"
#include <vector>

namespace origin
{
	class OpenGLMesh : public Mesh
	{
	public:
		OpenGLMesh() = default;
		OpenGLMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
		~OpenGLMesh() override;

		void Draw() override;
		bool IsLoaded() const override;

	private:
		std::shared_ptr<VertexArray> m_VertexArray;
		std::shared_ptr<VertexBuffer> m_VertexBuffer;

		bool m_Loaded = false;
	};
}