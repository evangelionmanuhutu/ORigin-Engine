// Copyright (c) Evangelion Manuhutu | ORigin Engine

#include "pch.h"
#include "Texture.h"
#include "Renderer.h"

#include "Platform/OpenGL/OpenGLTexture.h"

namespace origin {

	std::shared_ptr<Texture2D> Texture2D::Create(const std::filesystem::path& filepath, const TextureSpecification& specification)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:			return nullptr;
		case RendererAPI::API::OpenGL:		return std::make_shared<OpenGLTexture2D>(filepath, specification);
		}

		return nullptr;
	}

	std::shared_ptr<Texture2D> Texture2D::Create(const TextureSpecification& specification, Buffer data)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:			return nullptr;
		case RendererAPI::API::OpenGL:		return std::make_shared<OpenGLTexture2D>(specification, data);
		}

		return nullptr;
	}

	std::shared_ptr<TextureCube> TextureCube::Create(uint32_t width, uint32_t height)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:			return nullptr;
		case RendererAPI::API::OpenGL:		return std::make_shared<OpenGLTextureCube>(width, height);
		}

		return nullptr;
	}

	std::shared_ptr<TextureCube> TextureCube::Create(const std::string& filepath)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:			return nullptr;
		case RendererAPI::API::OpenGL:		return std::make_shared<OpenGLTextureCube>(filepath);
		}

		return nullptr;
	}
}