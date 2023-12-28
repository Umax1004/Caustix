module;

#if !defined(_MSC_VER)
#include <signal.h>
#endif

#include "spdlog/spdlog.h"
#include <source_location>

export module Foundation.Assert;

import Foundation.Log;

export namespace Caustix{
    template<typename... Args>
    void CASSERT(bool condition, const std::source_location location = std::source_location::current()) {
        if (!condition)
        {
            error("{}({})", location.file_name(), location.line());
#if defined (_MSC_VER)
            __debugbreak();
#else
            raise(SIGTRAP);
#endif
        }
    }
}
