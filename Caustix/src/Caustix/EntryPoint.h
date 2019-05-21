#pragma once

#ifdef CX_PLATFORM_WINDOWS

extern Caustix::Application* Caustix::CreateApplication();

int main(int argc, char** argv)
{
	auto app = Caustix::CreateApplication();
	app->Run();
	delete app;
}

#endif // CX_PLATFORM_WINDOWS
