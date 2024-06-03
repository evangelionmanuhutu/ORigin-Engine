// Copyright (c) Evangelion Manuhutu | ORigin Engine

#include "pch.h"
#include "Skybox.h"
#include "Origin\Renderer\Renderer.h"
#include "Origin\Renderer\RenderCommand.h"

#include "Platform\OpenGL\OpenGLSkybox.h"

#include "stb_image.h"

namespace origin
{
	std::shared_ptr<Skybox> Skybox::Create(const std::string& filepath, const std::string& format)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::None:		return nullptr;
			case RendererAPI::API::OpenGL:		return std::make_shared<OpenGLSkybox>(filepath, format);
		}
		return nullptr;
	}
}
