#pragma once

#include "Core.h"
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace Caustix
{
	class CAUSTIX_API Log
	{
	public:
		static void Init();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;

	};
}

// Core log macros
#define CX_CORE_TRACE(...)    ::Caustix::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define CX_CORE_INFO(...)     ::Caustix::Log::GetCoreLogger()->info(__VA_ARGS__)
#define CX_CORE_WARN(...)     ::Caustix::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define CX_CORE_ERROR(...)    ::Caustix::Log::GetCoreLogger()->error(__VA_ARGS__)
#define CX_CORE_FATAL(...)    ::Caustix::Log::GetCoreLogger()->fatal(__VA_ARGS__)

// Client log macros
#define CX_TRACE(...)	      ::Caustix::Log::GetClientLogger()->trace(__VA_ARGS__)
#define CX_INFO(...)	      ::Caustix::Log::GetClientLogger()->info(__VA_ARGS__)
#define CX_WARN(...)	      ::Caustix::Log::GetClientLogger()->warn(__VA_ARGS__)
#define CX_ERROR(...)	      ::Caustix::Log::GetClientLogger()->error(__VA_ARGS__)
#define CX_FATAL(...)	      ::Caustix::Log::GetClientLogger()->fatal(__VA_ARGS__)