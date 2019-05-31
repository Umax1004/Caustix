#include "cxpch.h"
#include "Shared.h"

namespace Caustix
{
	void VK_ERROR(VkResult result)
	{
		if (result < 0) {
			switch (result) {
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				CX_CORE_ERROR("VK_ERROR_OUT_OF_HOST_MEMORY");
				break;
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				CX_CORE_ERROR("VK_ERROR_OUT_OF_DEVICE_MEMORY");
				break;
			case VK_ERROR_INITIALIZATION_FAILED:
				CX_CORE_ERROR("VK_ERROR_INITIALIZATION_FAILED");
				break;
			case VK_ERROR_DEVICE_LOST:
				CX_CORE_ERROR("VK_ERROR_DEVICE_LOST");
				break;
			case VK_ERROR_MEMORY_MAP_FAILED:
				CX_CORE_ERROR("VK_ERROR_MEMORY_MAP_FAILED");
				break;
			case VK_ERROR_LAYER_NOT_PRESENT:
				CX_CORE_ERROR("VK_ERROR_LAYER_NOT_PRESENT");
				break;
			case VK_ERROR_EXTENSION_NOT_PRESENT:
				CX_CORE_ERROR("VK_ERROR_EXTENSION_NOT_PRESENT");
				break;
			case VK_ERROR_FEATURE_NOT_PRESENT:
				CX_CORE_ERROR("VK_ERROR_FEATURE_NOT_PRESENT");
				break;
			case VK_ERROR_INCOMPATIBLE_DRIVER:
				CX_CORE_ERROR("VK_ERROR_INCOMPATIBLE_DRIVER");
				break;
			case VK_ERROR_TOO_MANY_OBJECTS:
				CX_CORE_ERROR("VK_ERROR_TOO_MANY_OBJECTS");
				break;
			case VK_ERROR_FORMAT_NOT_SUPPORTED:
				CX_CORE_ERROR("VK_ERROR_FORMAT_NOT_SUPPORTED");
				break;
			case VK_ERROR_SURFACE_LOST_KHR:
				CX_CORE_ERROR("VK_ERROR_SURFACE_LOST_KHR");
				break;
			case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
				CX_CORE_ERROR("VK_ERROR_NATIVE_WINDOW_IN_USE_KHR");
				break;
			case VK_SUBOPTIMAL_KHR:
				CX_CORE_ERROR("VK_SUBOPTIMAL_KHR");
				break;
			case VK_ERROR_OUT_OF_DATE_KHR:
				CX_CORE_ERROR("VK_ERROR_OUT_OF_DATE_KHR");
				break;
			case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
				CX_CORE_ERROR("VK_ERROR_INCOMPATIBLE_DISPLAY_KHR");
				break;
			case VK_ERROR_VALIDATION_FAILED_EXT:
				CX_CORE_ERROR("VK_ERROR_VALIDATION_FAILED_EXT");
				break;
			default:
				break;
			}
			assert(0 && "Vulkan runtime error.");
		}
	}
}