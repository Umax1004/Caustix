#include <cxpch.h>
#include <Caustix.h>

class Sandbox : public Caustix::Application
{
public:

	Sandbox()
	{

	}

	~Sandbox()
	{

	}
};

Caustix::Application* Caustix::CreateApplication()
{
	return new Sandbox();
}