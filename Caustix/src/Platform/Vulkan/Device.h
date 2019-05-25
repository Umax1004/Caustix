#pragma once
#include <vulkan/vulkan.h>

namespace Caustix
{
	class Device
	{
	public:
		Device();
		~Device();

	private:

		void InitInstance();
	};
}