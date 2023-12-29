module;

#include <vector>
#include <memory>

#include <SDL.h>
#include <SDL_vulkan.h>

#include "imgui.h"
#include "imgui_impl_sdl2.h"

export module Application.Window;

import Foundation.Platform;
import Foundation.Memory.Allocators.Allocator;
import Foundation.Services.Service;
import Foundation.Log;
import Foundation.Numerics;

export namespace Caustix {

    struct WindowConfiguration {
        u32             width;
        u32             height;

        cstring         name;

        Allocator*      allocator;
    };

    typedef void        ( *OsMessagesCallback )( void* os_event, void* user_data );

    struct Window : public Service {
        ~Window() {
            Shutdown();
        }
        Window(WindowConfiguration configuration);

        void Shutdown();

        void            HandleOsMessages();

        void            SetFullscreen( bool value );

        void            RegisterOsMessagesCallback( OsMessagesCallback callback, void* userData );
        void            UnregisterOsMessagesCallback( OsMessagesCallback callback );

        void            CenterMouse( bool dragging );

        std::vector<OsMessagesCallback, STLAdaptor<OsMessagesCallback>> m_osMessagesCallbacks;
        std::vector<void*, STLAdaptor<void*>>                           m_osMessagesCallbacksData;

        void*           m_platformHandle    = nullptr;
        bool            m_requestedExit     = false;
        bool            m_resized           = false;
        bool            m_minimized         = false;
        u32             m_width             = 0;
        u32             m_height            = 0;
        f32             m_displayRefresh    = 1.0f / 60.0f;

        static Window* Create(WindowConfiguration configuration) {
            static std::unique_ptr<Window> instance{new Window(configuration)};
            return instance.get();
//            return nullptr;
        }

        static constexpr cstring    m_name = "caustix_window_service";

    };
}

namespace Caustix {
    static SDL_Window* window = nullptr;

    static f32 sdl_get_monitor_refresh() {
        SDL_DisplayMode current;
        int should_be_zero = SDL_GetCurrentDisplayMode( 0, &current );
        CASSERT( !should_be_zero );
        return 1.0f / current.refresh_rate;
    }

    void Window::Shutdown() {
        m_osMessagesCallbacksData.clear();
        m_osMessagesCallbacks.clear();

        SDL_DestroyWindow( window );
        SDL_Quit();

        info( "WindowService shutdown" );
    }

    Window::Window(WindowConfiguration configuration)
    : m_osMessagesCallbacks(*configuration.allocator)
    , m_osMessagesCallbacksData(*configuration.allocator) {
        info("WindowsService Initialized");

        if ( SDL_Init( SDL_INIT_EVERYTHING ) != 0 ) {
            error( "SDL Init error: {}", SDL_GetError() );
            return;
        }

        SDL_DisplayMode current;
        SDL_GetCurrentDisplayMode( 0, &current );

        SDL_WindowFlags window_flags = ( SDL_WindowFlags )( SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI );

        window = SDL_CreateWindow( configuration.name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, configuration.width, configuration.height, window_flags );

        info( "Window created successfully" );

        int window_width, window_height;
        SDL_Vulkan_GetDrawableSize( window, &window_width, &window_height );

        m_width = ( u32 )window_width;
        m_height = ( u32 )window_height;

        // Assing this so it can be accessed from outside.
        m_platformHandle = window;

        // Callbacks
        m_osMessagesCallbacks.reserve(4);
        m_osMessagesCallbacksData.reserve(4);

        m_displayRefresh = sdl_get_monitor_refresh();
    }

    void Window::HandleOsMessages() {
        SDL_Event event;
        while ( SDL_PollEvent( &event ) ) {

            ImGui_ImplSDL2_ProcessEvent( &event );

            switch ( event.type ) {
                case SDL_QUIT:
                {
                    m_requestedExit = true;
                    goto propagate_event;
                    break;
                }

                    // Handle subevent
                case SDL_WINDOWEVENT:
                {
                    switch ( event.window.event ) {
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                        case SDL_WINDOWEVENT_RESIZED:
                        {
                            // Resize only if even numbers are used?
                            // NOTE: This goes in an infinite loop when maximising a window that has an odd width/height.
                            /*if ( ( event.window.data1 % 2 == 1 ) || ( event.window.data2 % 2 == 1 ) ) {
                                u32 new_width = ( u32 )( event.window.data1 % 2 == 0 ? event.window.data1 : event.window.data1 - 1 );
                                u32 new_height = ( u32 )( event.window.data2 % 2 == 0 ? event.window.data2 : event.window.data2 - 1 );

                                if ( new_width != width || new_height != height ) {
                                    SDL_SetWindowSize( window, new_width, new_height );

                                    rprint( "Forcing resize to a multiple of 2, %ux%u from %ux%u\n", new_width, new_height, event.window.data1, event.window.data2 );
                                }
                            }*/
                            //else
                            {
                                u32 new_width = ( u32 )( event.window.data1 );
                                u32 new_height = ( u32 )( event.window.data2 );

                                // Update only if needed.
                                if ( new_width != m_width || new_height != m_height ) {
                                    m_resized = true;
                                    m_width = new_width;
                                    m_height = new_height;

                                    info( "Resizing to {}, {}", m_width, m_height );
                                }
                            }

                            break;
                        }

                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                        {
                            info( "Focus Gained" );
                            break;
                        }
                        case SDL_WINDOWEVENT_FOCUS_LOST:
                        {
                            info( "Focus Lost" );
                            break;
                        }
                        case SDL_WINDOWEVENT_MAXIMIZED:
                        {
                            info( "Maximized" );
                            m_minimized = false;
                            break;
                        }
                        case SDL_WINDOWEVENT_MINIMIZED:
                        {
                            info( "Minimized" );
                            m_minimized = true;
                            break;
                        }
                        case SDL_WINDOWEVENT_RESTORED:
                        {
                            info( "Restored" );
                            m_minimized = false;
                            break;
                        }
                        case SDL_WINDOWEVENT_TAKE_FOCUS:
                        {
                            info( "Take Focus" );
                            break;
                        }
                        case SDL_WINDOWEVENT_EXPOSED:
                        {
                            info( "Exposed" );
                            break;
                        }

                        case SDL_WINDOWEVENT_CLOSE:
                        {
                            m_requestedExit = true;
                            info( "Window close event received." );
                            break;
                        }
                        default:
                        {
                            m_displayRefresh = sdl_get_monitor_refresh();
                            break;
                        }
                    }
                    goto propagate_event;
                    break;
                }
            }
            // Maverick:
            propagate_event:
            // Callbacks
            for ( u32 i = 0; i < m_osMessagesCallbacks.size(); ++i ) {
                OsMessagesCallback callback = m_osMessagesCallbacks[ i ];
                callback( &event, m_osMessagesCallbacksData[i] );
            }
        }
    }

    void Window::SetFullscreen( bool value ) {
        if ( value )
            SDL_SetWindowFullscreen( window, SDL_WINDOW_FULLSCREEN_DESKTOP );
        else {
            SDL_SetWindowFullscreen( window, 0 );
        }
    }

    void Window::RegisterOsMessagesCallback(Caustix::OsMessagesCallback callback, void *userData) {
        m_osMessagesCallbacks.push_back( callback );
        m_osMessagesCallbacksData.push_back( userData );
    }

    void Window::UnregisterOsMessagesCallback(Caustix::OsMessagesCallback callback) {
        CASSERT(m_osMessagesCallbacks.size() < 8);
        if (m_osMessagesCallbacks.size() >= 8) {
            error("This array is too big for a linear search. Consider using something different!");
        }

        for ( u32 i = 0; i < m_osMessagesCallbacks.size(); ++i ) {
            if ( m_osMessagesCallbacks[ i ] == callback ) {
                m_osMessagesCallbacks.erase( m_osMessagesCallbacks.begin() + i );
                m_osMessagesCallbacksData.erase( m_osMessagesCallbacksData.begin() + i );
            }
        }
    }

    void Window::CenterMouse(bool dragging) {
        if ( dragging ) {
            SDL_WarpMouseInWindow( window, roundu32(m_width / 2.f), roundu32(m_height / 2.f) );
            SDL_SetWindowGrab( window, SDL_TRUE );
            SDL_SetRelativeMouseMode( SDL_TRUE );
        } else {
            SDL_SetWindowGrab( window, SDL_FALSE );
            SDL_SetRelativeMouseMode( SDL_FALSE );
        }
    }
}

