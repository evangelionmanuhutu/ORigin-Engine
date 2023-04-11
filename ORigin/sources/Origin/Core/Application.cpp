// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#include "pch.h"
#include "Application.h"

#include "Origin\Scripting\ScriptEngine.h"

namespace Origin {

	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& spec)
		: m_Specification(spec)
	{

		OGN_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		// Set working directory here
		if (!m_Specification.WorkingDirectory.empty())
			std::filesystem::current_path(m_Specification.WorkingDirectory);

		m_Window = Window::Create(m_Specification.Name);
		m_Window->SetEventCallback(OGN_BIND_EVENT_FN(Application::OnEvent));

		Renderer::Init();
		ScriptEngine::Init();

		m_GuiLayer = new GuiLayer();
		PushOverlay(m_GuiLayer);
	}

	Application::~Application()
	{
		ScriptEngine::Shutdown();
		Renderer::Shutdown();
	}

	void Application::Run()
	{
		m_Window->SetVSync(false);

		while (m_Window->Loop()) {

			auto time = (float)glfwGetTime();
			Timestep timestep = time - m_LastFrame;
			m_LastFrame = time;
			timestep.SetDeltaTime(m_LastFrame);

			ExecuteMainThreadQueue();

			if (!m_Minimized) {
				for (Layer* layer : m_LayerStack)
					layer->OnUpdate(timestep);
			}

			m_GuiLayer->Begin();
			for (Layer* layer : m_LayerStack)
				layer->OnGuiRender();
			m_GuiLayer->End();
			m_Window->OnUpdate();
		}
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(OGN_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(OGN_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (e.Handled)
				break;
			(*it)->OnEvent(e);
		}
	}

	void Application::SubmitToMainThread(const std::function<void()>& function)
	{
		std::scoped_lock<std::mutex> lock(m_MainThreadMutex);

		m_MainThreadQueue.emplace_back(function);
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		if(m_Window->Loop())
			return false;

		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return true;
		}
		m_Minimized = false;
		Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

		return false;
	}

	void Application::ExecuteMainThreadQueue()
	{
		for (auto& func : m_MainThreadQueue)
			func();

		m_MainThreadQueue.clear();
	}
}