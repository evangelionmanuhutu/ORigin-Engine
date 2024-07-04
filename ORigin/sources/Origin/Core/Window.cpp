// Copyright (c) Evangelion Manuhutu | ORigin Engine


#include "Window.h"

#ifdef OGN_PLATFORM_WINDOWS
	#include "Platform/Win32/Win32Window.h"
#endif

namespace origin
{
	std::shared_ptr<Window> Window::Create(const char* title,	uint32_t width,	uint32_t height, bool maximized)
	{
#ifdef OGN_PLATFORM_WINDOWS
		return std::make_shared<Win32Window>(title, width, height, maximized);
#else
		OGN_CORE_ASSERT(false, "Unkown Platform");
		return nullptr;
#endif
	}

	void Window::GLFWInit()
	{
		int success = glfwInit();
		OGN_CORE_ASSERT(success, "Failed to initialize GLFW");
	}

	void Window::GLFWShutdown() 
	{
		glfwTerminate();
	}

}