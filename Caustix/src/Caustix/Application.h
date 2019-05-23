#pragma once
#include "Core.h"
#include "Caustix/Layer/LayerStack.h"
#include "Caustix/Events/Event.h"
#include "Caustix/Events/ApplicationEvent.h"
#include "Window.h"

namespace Caustix
{
	class CAUSTIX_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

	private:
		std::unique_ptr<Window> m_Window;
		bool OnWindowClose(WindowCloseEvent& e);
		bool m_Running = true;
		LayerStack m_LayerStack;
	};

	Application* CreateApplication();
}

