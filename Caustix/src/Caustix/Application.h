#pragma once
#include "Core.h"
#include "Events/Event.h"

namespace Caustix
{
	class CAUSTIX_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
	};

	Application* CreateApplication();
}

