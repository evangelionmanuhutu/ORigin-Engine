
#include "Model.h"
#include "Renderer.h"

#include "Platform/OpenGL/OpenGLModel.h"

namespace origin
{
	std::shared_ptr<Model> Model::Create(const std::filesystem::path& filepath, Scene *scene)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::None:			return nullptr;
		case RendererAPI::API::OpenGL:		return std::make_shared<OpenGLModel>(filepath, scene);
		}

		return nullptr;
	}

}

