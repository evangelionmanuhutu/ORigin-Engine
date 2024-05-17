// Copyright (c) Evangelion Manuhutu | ORigin Engine

#pragma once
#include "Window.h"
#include "Input.h"
#include "AppEvent.h"
#include "Time.h"
#include "Layer.h"
#include "LayerStack.h"

#include "Origin\GUI\GuiLayer.h"
#include "Origin\Renderer\Renderer.h"
#include "Origin\Renderer\GraphicsContext.h"

#include <mutex>

namespace origin {

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
		ApplicationCommandLineArgs CommandLineArgs;
		std::string Name = "ORigin Application";
		std::string IconPath = "Resources/icon.png";
		std::string WorkingDirectory;

		uint32_t Width = 1280, Height = 640;
		bool Runtime = false;
		bool Maximize = false;
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

		void StartThreads();

		void SubmitToMainThread(const std::function<void()>& function);
		inline static Application& Get() { return *s_Instance; }
		inline bool GetMinimized() { return m_Minimized; }
		inline Window& GetWindow() { return *m_Window.get(); }
		const ApplicationSpecification& GetSpecification() const { return m_Spec; }
		GuiLayer* GetGuiLayer() { return m_GuiLayer; }
		bool SetVSync = false;

	private:
		ApplicationSpecification m_Spec;
		LayerStack m_LayerStack;
		std::unique_ptr<Window> m_Window;
		GuiLayer* m_GuiLayer, *m_SplashScreenGui;
		std::unique_ptr<Input> m_MainInputHandle;
		static Application* s_Instance;
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);
		void ExecuteMainThreadQueue();

		bool m_Minimized = false;
		float m_LastFrame = 0.0f;

		std::vector<std::function<void()>> m_MainThreadQueue;
		std::mutex m_MainThreadMutex;

	};

	Application* CreateApplication(ApplicationCommandLineArgs args);
}