#pragma once
#include <vulkan/vulkan.h>

namespace Caustix
{
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;

		bool isComplete() {
			return graphicsFamily.has_value();
		}
	};

	class Device
	{
	public:
		Device();
		~Device();

	private:


		VkInstance								instance				= VK_NULL_HANDLE;
		VkPhysicalDevice						physicalDevice			= VK_NULL_HANDLE;

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
		void DeInitInstance();

		void InitDevice();
		void DeInitDevice();

		void PickPhysicalDevice(std::vector<VkPhysicalDevice> gpuList);
		bool IsDeviceSuitable(VkPhysicalDevice device);
		int RateDeviceSuitability(VkPhysicalDevice device);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

		void AddRequiredPlatformInstanceExtensions(std::vector<const char *> *instance_extensions);
	};

	
}