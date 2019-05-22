#include "cxpch.h"
#include "Application.h"
#include "Events/ApplicationEvent.h"
#include "Log.h"

namespace Caustix
{

	Application::Application()
	{
	}


	Application::~Application()
	{
	}

	void Application::Run()
	{
		WindowResizeEvent e(1280, 720);
		if (e.IsInCategory(EventCategoryApplication))
		{
			CX_TRACE((int) e.GetStaticType());
			CX_TRACE(e);
		}
		if (e.IsInCategory(EventCategoryInput))
		{
			CX_TRACE(e);
		}

		while (true)
		{

		}
	}

}

