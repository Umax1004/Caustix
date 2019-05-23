#include <cxpch.h>
#include <Caustix.h>

class ExampleLayer : public Caustix::Layer
{
public:
	ExampleLayer()
		: Layer("Example")
	{
	}

	void OnUpdate() override
	{
		CX_INFO("ExampleLayer::Update");
	}

	void OnEvent(Caustix::Event& event) override
	{
		CX_TRACE("{0}", event);
	}

};

class Sandbox : public Caustix::Application
{
public:

	Sandbox()
	{
		PushLayer(new ExampleLayer());
	}

	~Sandbox()
	{

	}
};

Caustix::Application* Caustix::CreateApplication()
{
	return new Sandbox();
}