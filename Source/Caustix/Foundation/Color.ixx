export module Foundation.Color;

import Foundation.Platform;

export namespace Caustix {

    // Color class that embeds color in a uint32.
    struct Color {

        void    Set( float r, float g, float b, float a ) { abgr = u8( r * 255.f ) | ( u8( g * 255.f ) << 8 ) | ( u8( b * 255.f ) << 16 ) | ( u8( a * 255.f ) << 24 ); }

        f32     r() const                               { return ( abgr & 0xff ) / 255.f; }
        f32     g() const                               { return ( ( abgr >> 8 ) & 0xff ) / 255.f; }
        f32     b() const                               { return ( ( abgr >> 16 ) & 0xff ) / 255.f; }
        f32     a() const                               { return ( ( abgr >> 24 ) & 0xff ) / 255.f; }

        Color   operator=( const u32 color )            { abgr = color; return *this; }

        static u32  FromU8( u8 r, u8 g, u8 b, u8 a )       { return ( r | ( g << 8 ) | ( b << 16 ) | ( a << 24 ) ); }

        static u32  GetDistinctColor( u32 index );

        static const u32    red         = 0xff0000ff;
        static const u32    green       = 0xff00ff00;
        static const u32    blue        = 0xffff0000;
        static const u32    yellow      = 0xff00ffff;
        static const u32    black       = 0xff000000;
        static const u32    white       = 0xffffffff;
        static const u32    transparent = 0x00000000;

        u32     abgr;
    };
}

namespace Caustix {

    // 64 Distinct Colors. Used for graphs and anything that needs random colors.
    //
    static const u32 kDistinctColors[] = {
            0xFF000000, 0xFF00FF00, 0xFFFF0000, 0xFF0000FF, 0xFFFEFF01, 0xFFFEA6FF, 0xFF66DBFF, 0xFF016400,
            0xFF670001, 0xFF3A0095, 0xFFB57D00, 0xFFF600FF, 0xFFE8EEFF, 0xFF004D77, 0xFF92FB90, 0xFFFF7600,
            0xFF00FFD5, 0xFF7E93FF, 0xFF6C826A, 0xFF9D02FF, 0xFF0089FE, 0xFF82477A, 0xFFD22D7E, 0xFF00A985,
            0xFF5600FF, 0xFF0024A4, 0xFF7EAE00, 0xFF3B3D68, 0xFFFFC6BD, 0xFF003426, 0xFF93D3BD, 0xFF17B900,
            0xFF8E009E, 0xFF441500, 0xFF9F8CC2, 0xFFA374FF, 0xFFFFD001, 0xFF544700, 0xFFFE6FE5, 0xFF318278,
            0xFFA14C0E, 0xFFCBD091, 0xFF7099BE, 0xFFE88A96, 0xFF0088BB, 0xFF2C0043, 0xFF74FFDE, 0xFFC6FF00,
            0xFF02E5FF, 0xFF000E62, 0xFF9C8F00, 0xFF52FF98, 0xFFB14475, 0xFFFF00B5, 0xFF78FF00, 0xFF416EFF,
            0xFF395F00, 0xFF82686B, 0xFF4EAD5F, 0xFF4057A7, 0xFFD2FFA5, 0xFF67B1FF, 0xFFFF9B00, 0xFFBE5EE8
    };

    u32 Color::GetDistinctColor(u32 index) {
        return kDistinctColors[ index % 64 ];
    }

}