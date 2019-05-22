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

#define BIT(x) (1 << x)