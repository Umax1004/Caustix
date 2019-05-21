#pragma once

#include "Core.h"

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

