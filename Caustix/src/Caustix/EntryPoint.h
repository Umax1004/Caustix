#pragma once

#ifdef CX_PLATFORM_WINDOWS

extern Caustix::Application* Caustix::CreateApplication();

int main(int argc, char** argv)
{
	Caustix::Log::Init();
	CX_CORE_WARN("Initialized Logger");
	int a = 42;
	CX_INFO("Hello world {0}",a);

	auto app = Caustix::CreateApplication();
	app->Run();
	delete app;
}

#endif // CX_PLATFORM_WINDOWS
