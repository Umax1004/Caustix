module;

#include "spdlog/spdlog.h"

export module Foundation.Log;

export namespace Caustix {
    template<typename... Args>
    void error(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        spdlog::error(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(spdlog::format_string_t<Args...> fmt, Args &&...args) {
        spdlog::info(fmt, std::forward<Args>(args)...);
    }
}