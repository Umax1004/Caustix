module;

#include <cmath>

export module Foundation.Numerics;

export import Foundation.Platform;
import Foundation.Assert;
import Foundation.Log;

export namespace Caustix {

    // Math utils /////////////////////////////////////////////////////////////////////////////////
    // Conversions from float/double to int/uint
    //
    // Define this macro to check if converted value can be contained in the destination int/uint.
    #define CAUSTIX_MATH_OVERFLOW_CHECK

    // Undefine the macro versions of this.
    #undef max
    #undef min

    template <typename T>
    T                               max( const T& a, const T& b ) { return a > b ? a : b; }

    template <typename T>
    T                               min( const T& a, const T& b ) { return a < b ? a : b; }

    template <typename T>
    T                               clamp( const T& v, const T& a, const T& b ) { return v < a ? a : (v > b ? b : v); }

    template <typename To, typename From>
    To safe_cast( From a ) {
        To result = ( To )a;

        From check = ( From )result;
        RASSERT( check == result );

        return result;
    }

    u32                             ceilu32( f32 value );
    u32                             ceilu32( f64 value );
    u16                             ceilu16( f32 value );
    u16                             ceilu16( f64 value );
    i32                             ceili32( f32 value );
    i32                             ceili32( f64 value );
    i16                             ceili16( f32 value );
    i16                             ceili16( f64 value );

    u32                             flooru32( f32 value );
    u32                             flooru32( f64 value );
    u16                             flooru16( f32 value );
    u16                             flooru16( f64 value );
    i32                             floori32( f32 value );
    i32                             floori32( f64 value );
    i16                             floori16( f32 value );
    i16                             floori16( f64 value );

    u32                             roundu32( f32 value );
    u32                             roundu32( f64 value );
    u16                             roundu16( f32 value );
    u16                             roundu16( f64 value );
    i32                             roundi32( f32 value );
    i32                             roundi32( f64 value );
    i16                             roundi16( f32 value );
    i16                             roundi16( f64 value );

    f32 GetRandomValue( f32 min, f32 max );

    const f32 rpi = 3.1415926538f;
    const f32 rpi_2 = 1.57079632679f;
}

namespace Caustix {
    #if defined (CAUSTIX_MATH_OVERFLOW_CHECK)

    //
    // Use an integer 64 to see if the value converted is overflowing.
    //
    #define hy_math_convert(val, max, type, func)\
        i64 value_container = (i64)func(value);\
        if (abs(value_container) > max)\
            error( "Overflow converting values {}, {}", value_container, max );\
        const type v = (type)value_container;


    #define hy_math_func_f32(max, type, func)\
        type func##type( f32 value ) {\
            hy_math_convert( value, max, type, func##f );\
            return v;\
        }\

    #define hy_math_func_f64(max, type, func)\
        type func##type( f64 value ) {\
            hy_math_convert( value, max, type, func );\
            return v;\
        }\

    #else
    #define hy_math_convert(val, max, type, func)\
            (type)func(value);

    #define hy_math_func_f32(max, type, func)\
        type func##type( f32 value ) {\
            return hy_math_convert( value, max, type, func##f );\
        }\

    #define hy_math_func_f64(max, type, func)\
        type func##type( f64 value ) {\
            return hy_math_convert( value, max, type, func );\
        }\

    #endif

    //
    // Avoid double typeing functions for float and double
    //
    #define hy_math_func_f32_f64(max, type, func)\
        hy_math_func_f32( max, type, func )\
        hy_math_func_f64( max, type, func )\

    // Function declarations //////////////////////////////////////////////////////////////////////////

    // Ceil
    hy_math_func_f32_f64( UINT32_MAX, u32, ceil )
    hy_math_func_f32_f64( UINT16_MAX, u16, ceil )
    hy_math_func_f32_f64( INT32_MAX, i32, ceil )
    hy_math_func_f32_f64( INT16_MAX, i16, ceil )

    // Floor
    hy_math_func_f32_f64( UINT32_MAX, u32, floor )
    hy_math_func_f32_f64( UINT16_MAX, u16, floor )
    hy_math_func_f32_f64( INT32_MAX, i32, floor )
    hy_math_func_f32_f64( INT16_MAX, i16, floor )

    // Round
    hy_math_func_f32_f64( UINT32_MAX, u32, round )
    hy_math_func_f32_f64( UINT16_MAX, u16, round )
    hy_math_func_f32_f64( INT32_MAX, i32, round )
    hy_math_func_f32_f64( INT16_MAX, i16, round )

    f32 GetRandomValue( f32 min, f32 max ) {
        CASSERT( min < max );

        f32 rnd = ( f32 )rand() / ( f32 )RAND_MAX;

        rnd = ( max - min ) * rnd + min;

        return rnd;
    }

}
