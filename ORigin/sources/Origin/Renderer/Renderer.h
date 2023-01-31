// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#pragma once
#include "RenderCommand.h"

#include "Origin\Scene\Component/Camera.h"
#include "Origin\Scene\EditorCamera.h"
#include "Origin\Renderer\UniformBuffer.h"

namespace Origin
{
	struct RendererData
	{
		struct CameraData
		{
			glm::mat4 ViewProjection;
		};
		CameraData CameraBufferData;
		std::shared_ptr<UniformBuffer> CameraUniformBuffer;

		struct LightingData
		{
			glm::vec3 Position;
			glm::vec4 Color;
			float Intensity;
		};
		LightingData LightingBufferData;
		std::shared_ptr<UniformBuffer> LightingUniformBuffer;
	};

	static RendererData s_RendererData;

	class Renderer
	{
	public:
		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

		static void Init();
		static void Shutdown();

		static void BeginScene(const Camera& camera, const glm::mat4& transform);
		static void BeginScene(const EditorCamera& camera);

		static void EndScene();

		static void OnWindowResize(uint32_t width, uint32_t height);
		static void OnUpdate();
		void DrawLineMode(bool enable);
	};
}