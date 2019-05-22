#pragma once

#ifdef CX_PLATFORM_WINDOWS
	#ifdef CX_BUILD_DLL
		#define CAUSTIX_API _declspec(dllexport)
	#else
		#define CAUSTIX_API _declspec(dllimport)
	#endif // CX_BUILD_DLL
#else
	#error Caustix only supports windows for now :(
#endif // CX_PLATFORM_WINDOWS

#ifdef CX_ENABLE_ASSERTS
#define CX_ASSERT(x, ...) { if(!(x)) { CX_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#define CX_CORE_ASSERT(x, ...) { if(!(x)) { CX_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
#else
#define CX_ASSERT(x, ...)
#define CX_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)