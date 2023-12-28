module;

#include <cstdint>

export module Foundation.Platform;

export {
    // Native types typedefs /////////////////////////////////////////////////
    using u8                = uint8_t;
    using u16               = uint16_t;
    using u32               = uint32_t;
    using u64               = uint64_t;

    using i8                = int8_t;
    using i16               = int16_t;
    using i32               = int32_t;
    using i64               = int64_t;

    using f32               = float;
    using f64               = double;

    using sizet             = size_t;

    using cstring           = const char*;

    constexpr u64   u64_max = UINT64_MAX;
    constexpr i64   i64_max = INT64_MAX;
    constexpr u32   u32_max = UINT32_MAX;
    constexpr i32   i32_max = INT32_MAX;
    constexpr u16   u16_max = UINT16_MAX;
    constexpr i16   i16_max = INT16_MAX;
    constexpr u8     u8_max = UINT8_MAX;
    constexpr i8     i8_max = INT8_MAX;

    // Helper Templates ////////////////////////////////////////////////////////////////

    template <typename T, u32 N>
    constexpr u32 ArraySize(const T(&)[N]) noexcept {
        return N;
    }
}