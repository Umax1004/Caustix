#pragma once

#include <stdint.h>

#if !defined(_MSC_VER)
#include <signal.h>
#endif

// Macros ////////////////////////////////////////////////////////////////

#if defined (_MSC_VER)
#define RAPTOR_INLINE                               inline
#define RAPTOR_FINLINE                              __forceinline
#define RAPTOR_DEBUG_BREAK                          __debugbreak();
#define RAPTOR_DISABLE_WARNING(warning_number)      __pragma( warning( disable : warning_number ) )
#define RAPTOR_CONCAT_OPERATOR(x, y)                x##y
#else
#define RAPTOR_INLINE                               inline
#define RAPTOR_FINLINE                              always_inline
#define RAPTOR_DEBUG_BREAK                          raise(SIGTRAP);
#define RAPTOR_CONCAT_OPERATOR(x, y)                x y
#endif // MSVC

#define RAPTOR_STRINGIZE( L )                       #L 
#define RAPTOR_MAKESTRING( L )                      RAPTOR_STRINGIZE( L )
#define RAPTOR_CONCAT(x, y)                         RAPTOR_CONCAT_OPERATOR(x, y)
#define RAPTOR_LINE_STRING                          RAPTOR_MAKESTRING( __LINE__ ) 
#define RAPTOR_FILELINE(MESSAGE)                    __FILE__ "(" RAPTOR_LINE_STRING ") : " MESSAGE

// Unique names
#define RAPTOR_UNIQUE_SUFFIX(PARAM)                 RAPTOR_CONCAT(PARAM, __LINE__ )

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