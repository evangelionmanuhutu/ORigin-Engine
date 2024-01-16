// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#include "pch.h"
#include "Renderer.h"
#include "RenderCommand.h"

#include "Origin\Asset\AssetImporter.h"

#include "Origin\Scene\Skybox.h"
#include "Platform\OpenGL\OpenGL_Shader.h"

#include "Renderer2D.h"
#include "Renderer3D.h"

#include <glm\gtc\matrix_transform.hpp>

namespace origin {

#define CAMERA_BINDING_POINT 0

	static ShaderLibrary GShaderLibrary;
	static Renderer::CameraBuffer s_CameraBuffer;
	static std::shared_ptr<UniformBuffer> s_CameraUBO;

	std::shared_ptr<Texture2D> Renderer::WhiteTexture;

	bool Renderer::Init()
	{
		RenderCommand::Init();

		Renderer::LoadShader();
		Renderer2D::Init();
		Renderer3D::Init();

		WhiteTexture = Texture2D::Create(TextureSpecification());
		uint32_t whiteTextureData = 0xffffffff;
		WhiteTexture->SetData(Buffer(&whiteTextureData, sizeof(uint32_t)));

		s_CameraUBO = UniformBuffer::Create(sizeof(s_CameraBuffer), CAMERA_BINDING_POINT);

		return true;
	}

	void Renderer::Shutdown()
	{
		Renderer2D::Shutdown();
		Renderer3D::Shutdown();

		OGN_CORE_WARN("Renderer Shutdown");
	}

	void Renderer::BeginScene(const SceneCamera& camera, const TransformComponent& cameraTransform)
	{
		s_CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(cameraTransform.GetTransform());
		s_CameraBuffer.Position = camera.GetPosition();

		s_CameraUBO->SetData(&s_CameraBuffer, sizeof(s_CameraBuffer));

		Renderer2D::Begin();
		Renderer3D::Begin();
	}

	void Renderer::BeginScene(const EditorCamera& camera)
	{
		s_CameraBuffer.ViewProjection = camera.GetViewProjection();
		s_CameraBuffer.Position = camera.GetPosition();

		s_CameraUBO->SetData(&s_CameraBuffer, sizeof(s_CameraBuffer));

		Renderer2D::Begin();
		Renderer3D::Begin();
	}

	void Renderer::EndScene()
	{
		Renderer2D::End();
		Renderer3D::End();

		s_CameraUBO->Unbind();
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::OnUpdate()
	{
		RenderCommand::ClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		RenderCommand::Clear();
	}

	std::shared_ptr<Shader> Renderer::GetGShader(const std::string& name)
	{
		return GShaderLibrary.Get(name);
	}

	const std::unordered_map<std::string, std::shared_ptr<Shader>> Renderer::GetSaderLibrary()
	{
		return GShaderLibrary.GetMap();
	}

	Renderer::CameraBuffer Renderer::GetCameraBuffer()
	{
		return s_CameraBuffer;
	}

	void Renderer::LoadShader()
	{
		bool recompileShader = false;

		GShaderLibrary.Load("Line2D", "Resources/Shaders/SPIR-V/Line2D.glsl", true, recompileShader);
		GShaderLibrary.Load("Circle2D", "Resources/Shaders/SPIR-V/Circle2D.glsl", true, recompileShader);
		GShaderLibrary.Load("Quad2D", "Resources/Shaders/SPIR-V/Quad2D.glsl", true, recompileShader);
		GShaderLibrary.Load("Text", "Resources/Shaders/SPIR-V/TextRenderer.glsl", true, recompileShader);
		GShaderLibrary.Load("Cube", "Resources/Shaders/SPIR-V/Cube.glsl", true, recompileShader);
		GShaderLibrary.Load("Skybox", "Resources/Shaders/Skybox.glsl", false);
	}
}