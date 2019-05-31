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


		VkInstance	instance = VK_NULL_HANDLE;

		std::vector<const char*> instanceLayers;
		std::vector<const char*> instanceExtensions;
		//std::vector<const char*> deviceLayers; Depricated
		std::vector<const char*> deviceExtensions;

		//Debug Vars
		VkDebugReportCallbackEXT debugReport = nullptr;
		VkDebugReportCallbackCreateInfoEXT debugCallbackCreateInfo{};

		//Functions
		void SetupLayersAndExtensions();

		//Debug Setup
		void SetupDebug();
		void InitDebug();
		void DeInitDebug();

		void InitInstance();
		void AddRequiredPlatformInstanceExtensions(std::vector<const char *> *instance_extensions);
	};
}