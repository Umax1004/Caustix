#include "cxpch.h"
#include "Application.h"
#include "Events/ApplicationEvent.h"
#include "Log.h"

#include <GLFW/glfw3.h>

namespace Caustix
{

	Application::Application()
	{
		m_Window = std::unique_ptr<Window>(Window::Create());
	}


	Application::~Application()
	{
	}

	void Application::Run()
	{

		while (true)
		{
			m_Window->OnUpdate();
		}
	}

}

