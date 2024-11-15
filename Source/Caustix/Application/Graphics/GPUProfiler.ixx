module;

#include <unordered_map>

#include <imgui.h>

export module Application.Graphics.GPUProfiler;

import Application.Graphics.GPUDevice;

import Foundation.Platform;
import Foundation.Memory.Allocators.Allocator;
import Foundation.Memory.MemoryDefines;
import Foundation.Color;
import Foundation.Numerics;

export namespace Caustix {
    #define FlatHashMap(key, value) std::unordered_map<key, value, std::hash<key>, std::equal_to<key>, STLAdaptor<std::pair<const key, value>>>
    struct GPUProfiler {
        GPUProfiler( Allocator* allocator, u32 maxFrames );
        ~GPUProfiler();

        void                        Update( GpuDevice& gpu );

        void                        ImguiDraw();

        Allocator*                  m_allocator;
        GPUTimestamp*               m_timestamps;
        u16*                        m_perFrameActive;

        u32                         m_maxFrames;
        u32                         m_currentFrame;

        f32                         m_maxTime;
        f32                         m_minTime;
        f32                         m_averageTime;

        f32                         m_maxDuration;
        bool                        m_paused;

    private:
        // GPU task names to colors
        FlatHashMap(u64, u32)   m_nameToColor;
    };
}

namespace Caustix {

    static u32      initial_frames_paused = 3;

    GPUProfiler::GPUProfiler( Allocator* allocator, u32 maxFrames )
    : m_allocator(allocator)
    , m_maxFrames(maxFrames)
    , m_nameToColor(*allocator) {

        m_timestamps = ( GPUTimestamp* )calloca( sizeof( GPUTimestamp ) * maxFrames * 32, allocator );
        m_perFrameActive = ( u16* )calloca( sizeof( u16 ) * maxFrames, allocator );

        m_maxDuration = 16.666f;
        m_currentFrame = 0;
        m_minTime = m_maxTime = m_averageTime = 0.f;
        m_paused = false;

        memset( m_perFrameActive, 0, 2 * maxFrames );

        m_nameToColor.reserve( 16 );
//        name_to_color.set_default_value( u32_max );
    }

    GPUProfiler::~GPUProfiler() {

        m_nameToColor.clear();

        cfree( m_timestamps, m_allocator );
        cfree( m_perFrameActive, m_allocator );
    }

    u32 GetWithDef(const  FlatHashMap(u64, u32)& m, const u64& key, const u32& defval ) {
        typename FlatHashMap(u64, u32)::const_iterator it = m.find( key );
        if ( it == m.end() ) {
            return defval;
        }
        else {
            return it->second;
        }
    }

    void GPUProfiler::Update( GpuDevice& gpu ) {

        gpu.set_gpu_timestamps_enable( !m_paused );

        if ( initial_frames_paused ) {
            --initial_frames_paused;
            return;
        }

        if ( m_paused && !gpu.resized )
            return;

        u32 active_timestamps = gpu.get_gpu_timestamps( &m_timestamps[ 32 * m_currentFrame ] );
        m_perFrameActive[ m_currentFrame ] = ( u16 )active_timestamps;

        // Get colors
        for ( u32 i = 0; i < active_timestamps; ++i ) {
            GPUTimestamp& timestamp = m_timestamps[ 32 * m_currentFrame + i ];

            u64 hashed_name = HashCalculate( timestamp.m_name );
            u32 color_index = GetWithDef(m_nameToColor, hashed_name, u32_max );
            // No entry found, add new color
            if ( color_index == u32_max ) {

                color_index = ( u32 )m_nameToColor.size();
                m_nameToColor.emplace( hashed_name, color_index );
            }

            timestamp.m_color = Color::GetDistinctColor( color_index );
        }

        m_currentFrame = ( m_currentFrame + 1 ) % m_maxFrames;

        // Reset Min/Max/Average after few frames
        if ( m_currentFrame == 0 ) {
            m_maxTime = -FLT_MAX;
            m_minTime = FLT_MAX;
            m_averageTime = 0.f;
        }
    }

void GPUProfiler::ImguiDraw() {
    if ( initial_frames_paused ) {
        return;
    }

    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImGui::GetContentRegionAvail();
        f32 widget_height = canvas_size.y - 100;

        f32 legend_width = 200;
        f32 graph_width = fabsf( canvas_size.x - legend_width );
        u32 rect_width = ceilu32( graph_width / m_maxFrames );
        i32 rect_x = ceili32( graph_width - rect_width );

        f64 new_average = 0;

        ImGuiIO& io = ImGui::GetIO();

        static char buf[ 128 ];

        const ImVec2 mouse_pos = io.MousePos;

        i32 selected_frame = -1;

        // Draw time reference lines
        sprintf( buf, "%3.4fms", m_maxDuration);
        draw_list->AddText( { cursor_pos.x, cursor_pos.y }, 0xff0000ff, buf );
        draw_list->AddLine( { cursor_pos.x + rect_width, cursor_pos.y }, { cursor_pos.x + graph_width, cursor_pos.y }, 0xff0000ff );

        sprintf( buf, "%3.4fms", m_maxDuration/ 2.f );
        draw_list->AddText( { cursor_pos.x, cursor_pos.y + widget_height / 2.f }, 0xff00ffff, buf );
        draw_list->AddLine( { cursor_pos.x + rect_width, cursor_pos.y + widget_height / 2.f }, { cursor_pos.x + graph_width, cursor_pos.y + widget_height / 2.f }, 0xff00ffff );

        // Draw Graph
        for ( u32 i = 0; i < m_maxFrames; ++i ) {
            u32 frame_index = ( m_currentFrame - 1 - i ) % m_maxFrames;

            f32 frame_x = cursor_pos.x + rect_x;
            GPUTimestamp* frame_timestamps = &m_timestamps[ frame_index * 32 ];
            f32 frame_time = ( f32 )frame_timestamps[ 0 ].m_elapsedms;
            // Clamp values to not destroy the frame data
            frame_time = clamp( frame_time, 0.00001f, 1000.f );
            // Update timings
            new_average += frame_time;
            m_minTime = min( m_minTime, frame_time );
            m_maxTime = max( m_maxTime, frame_time );

            f32 rect_height = frame_time / m_maxDuration* widget_height;
            //drawList->AddRectFilled( { frame_x, cursor_pos.y + rect_height }, { frame_x + rect_width, cursor_pos.y }, 0xffffffff );

            for ( u32 j = 0; j < m_perFrameActive[ frame_index ]; ++j ) {
                const GPUTimestamp& timestamp = frame_timestamps[ j ];

                /*if ( timestamp.depth != 1 ) {
                    continue;
                }*/

                rect_height = ( f32 )timestamp.m_elapsedms / m_maxDuration* widget_height;
                draw_list->AddRectFilled( { frame_x, cursor_pos.y + widget_height - rect_height },
                                          { frame_x + rect_width, cursor_pos.y + widget_height }, timestamp.m_color );
            }

            if ( mouse_pos.x >= frame_x && mouse_pos.x < frame_x + rect_width &&
                 mouse_pos.y >= cursor_pos.y && mouse_pos.y < cursor_pos.y + widget_height ) {
                draw_list->AddRectFilled( { frame_x, cursor_pos.y + widget_height },
                                          { frame_x + rect_width, cursor_pos.y }, 0x0fffffff );

                ImGui::SetTooltip( "(%u): %f", frame_index, frame_time );

                selected_frame = frame_index;
            }

            draw_list->AddLine( { frame_x, cursor_pos.y + widget_height }, { frame_x, cursor_pos.y }, 0x0fffffff );

            // Debug only
            /*static char buf[ 32 ];
            sprintf( buf, "%u", frame_index );
            draw_list->AddText( { frame_x, cursor_pos.y + widget_height - 64 }, 0xffffffff, buf );

            sprintf( buf, "%u", frame_timestamps[0].frame_index );
            drawList->AddText( { frame_x, cursor_pos.y + widget_height - 32 }, 0xffffffff, buf );*/

            rect_x -= rect_width;
        }

        m_averageTime = ( f32 )new_average / m_maxFrames;

        // Draw legend
        ImGui::SetCursorPosX( cursor_pos.x + graph_width );
        // Default to last frame if nothing is selected.
        selected_frame = selected_frame == -1 ? ( m_currentFrame - 1 ) % m_maxFrames : selected_frame;
        if ( selected_frame >= 0 ) {
            GPUTimestamp* frame_timestamps = &m_timestamps[ selected_frame * 32 ];

            f32 x = cursor_pos.x + graph_width;
            f32 y = cursor_pos.y;

            for ( u32 j = 0; j < m_perFrameActive[ selected_frame ]; ++j ) {
                const GPUTimestamp& timestamp = frame_timestamps[ j ];

                /*if ( timestamp.depth != 1 ) {
                    continue;
                }*/

                draw_list->AddRectFilled( { x, y },
                                          { x + 8, y + 8 }, timestamp.m_color );

                sprintf( buf, "(%d)-%s %2.4f", timestamp.m_depth, timestamp.m_name, timestamp.m_elapsedms );
                draw_list->AddText( { x + 12, y }, 0xffffffff, buf );

                y += 16;
            }
        }

        ImGui::Dummy( { canvas_size.x, widget_height } );
    }

    ImGui::SetNextItemWidth( 100.f );
    ImGui::LabelText( "", "Max %3.4fms", m_maxTime );
    ImGui::SameLine();
    ImGui::SetNextItemWidth( 100.f );
    ImGui::LabelText( "", "Min %3.4fms", m_minTime );
    ImGui::SameLine();
    ImGui::LabelText( "", "Ave %3.4fms", m_averageTime );

    ImGui::Separator();
    ImGui::Checkbox( "Pause", &m_paused );

    static const char* items[] = { "200ms", "100ms", "66ms", "33ms", "16ms", "8ms", "4ms" };
    static const float max_durations[] = { 200.f, 100.f, 66.f, 33.f, 16.f, 8.f, 4.f };

    static int max_duration_index = 4;
    if ( ImGui::Combo( "Graph Max", &max_duration_index, items, IM_ARRAYSIZE( items ) ) ) {
        m_maxDuration= max_durations[ max_duration_index ];
    }
}
}