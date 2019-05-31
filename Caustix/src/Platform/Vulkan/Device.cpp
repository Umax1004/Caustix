#include "cxpch.h"
#include "Device.h"
#include <GLFW/glfw3.h>
#include "Shared.h"

namespace Caustix
{
	Device::Device()
	{
		SetupLayersAndExtensions();
		SetupDebug();
		InitInstance();
		InitDebug();

	}

	Device::~Device()
	{
		DeInitDebug();
	}

	void Device::SetupLayersAndExtensions()
	{
		instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		AddRequiredPlatformInstanceExtensions(&instanceExtensions);
	}

	

	void Device::InitInstance()
	{
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Sandbox Application";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Caustix Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_1;

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &appInfo;
		instanceCreateInfo.enabledLayerCount = instanceLayers.size();
		instanceCreateInfo.ppEnabledLayerNames = instanceLayers.data();
		instanceCreateInfo.enabledExtensionCount = instanceExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

		VK_ERROR(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));
	}

#if VK_USE_PLATFORM_WIN32_KHR
	void Device::AddRequiredPlatformInstanceExtensions(std::vector<const char*>* instance_extensions)
	{
		instance_extensions->push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	}
#else
	void Device::AddRequiredPlatformInstanceExtensions(std::vector<const char*>* instance_extensions)
	{
	}
#endif // VK_USE_PLATFORM_WIN32_KHR

#if CX_DEBUG

	VKAPI_ATTR VkBool32 VKAPI_CALL
		VulkanDebugCallback(
			VkDebugReportFlagsEXT flags,
			VkDebugReportObjectTypeEXT obj_flags,
			uint64_t src_obj,
			size_t location,
			int32_t msg_code,
			const char* layer_prefix,
			const char* msg,
			void * user_data
		)
	{
		std::ostringstream stream;
		stream << "VKDBG: ";
		if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
			stream << "INFO: ";
		}
		if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
			stream << "WARNING: ";
		}
		if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
			stream << "PERFORMANCE: ";
		}
		if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
			stream << "ERROR: ";
		}
		if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
			stream << "DEBUG: ";
		}
		stream << "@[" << layer_prefix << "]: ";
		stream << msg ;
		CX_CORE_TRACE(stream.str());

#if CX_PLATFORM_WINDOWS
		if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) 
		{
			MessageBox(NULL, (LPCWSTR)stream.str().c_str(), (LPCWSTR) "Vulkan Error!", 0);
		}
#endif // CX_PLATFORM_WINDOWS

		return false;
	}
	void Device::SetupDebug()
	{
		debugCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
		debugCallbackCreateInfo.pfnCallback = VulkanDebugCallback;
		debugCallbackCreateInfo.flags =
			VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
			VK_DEBUG_REPORT_WARNING_BIT_EXT |
			VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
			VK_DEBUG_REPORT_ERROR_BIT_EXT |
			VK_DEBUG_REPORT_DEBUG_BIT_EXT |
			//VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT | 0;
			0;

		instanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
		instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	PFN_vkCreateDebugReportCallbackEXT		fvkCreateDebugReportCallbackEXT = nullptr;
	PFN_vkDestroyDebugReportCallbackEXT		fvkDestroyDebugReportCallbackEXT = nullptr;

	void Device::InitDebug()
	{
		fvkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		fvkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
		if (nullptr == fvkCreateDebugReportCallbackEXT || nullptr == fvkDestroyDebugReportCallbackEXT)
		{
			assert(0 && "Vulkan ERROR: Can't fetch debug function pointer");
			std::exit(-1);
		}

		fvkCreateDebugReportCallbackEXT(instance, &debugCallbackCreateInfo, nullptr, &debugReport);
	}

	void Device::DeInitDebug()
	{
		fvkDestroyDebugReportCallbackEXT(instance, debugReport, nullptr);
		debugReport = VK_NULL_HANDLE;
	}

#else
	void Device::SetupDebug()
	{
		instanceLayers.push_back("VK_LAYER_LUNARG_standard_validation");
		instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	void Device::InitDebug()
	{
	}

	void Device::DeInitDebug()
	{
	}
#endif // CX_DEBUG


}