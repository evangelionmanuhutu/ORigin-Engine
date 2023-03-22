// Copyright (c) 2022 Evangelion Manuhutu | ORigin Engine

#pragma once
#include "OriginCore.h"
#include "Window.h"

#include "Origin\IO\Input.h"
#include "Origin\IO\Events\AppEvent.h"

#include "Origin\Utils\Time.h"
#include "Origin\Utils\Layer.h"
#include "Origin\Utils\LayerStack.h"
#include "Origin\Utils\GUI\GuiLayer.h"

#include "Origin\Renderer\Renderer.h"

namespace Origin {

	struct ApplicationCommandLineArgs
	{
		int Count = 0;
		char** Args = nullptr;

		const char* operator[](int index) const {
			if (index > Count) __debugbreak();
			return Args[index];
		}
	};

	struct ApplicationSpecification
	{
		std::string Name = "ORigin Application";
		std::string WorkingDirectory;
		ApplicationCommandLineArgs CommandLineArgs;
	};

	class Application
	{
	public:
		Application(const ApplicationSpecification& spec);

		virtual ~Application();

		void OnEvent(Event& e);
		void Run();
		void Close() { m_Window->SetClose(true); }

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		bool SetVSync;

		inline static Application& Get() { return *s_Instance; }
		inline bool GetMinimized() { return m_Minimized; }
		inline Window& GetWindow() { return *m_Window; }
		const ApplicationSpecification& GetSpecification() const { return m_Specification; }
		GuiLayer* GetGuiLayer() { return m_GuiLayer; }

	private:
		ApplicationSpecification m_Specification;
		static Application* s_Instance;
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		bool m_Minimized = false;
		LayerStack m_LayerStack;
		std::unique_ptr<Window> m_Window;
		GuiLayer* m_GuiLayer;
		float m_LastFrame = 0.0f;
	};

	Application* CreateApplication(ApplicationCommandLineArgs args);
}