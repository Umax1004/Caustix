module;

#include <memory>
#include <vector>
#include <string>

#include "imgui.h"

#define INPUT_BACKEND_SDL

#if defined (INPUT_BACKEND_SDL)
#include <SDL.h>
#endif // INPUT_BACKEND_SDL

export module Application.Input;

export import Application.Keys;

import Foundation.Memory.Allocators.Allocator;
import Foundation.Services.Service;
import Foundation.Platform;
import Foundation.Log;

export namespace Caustix {
    constexpr u32 k_max_gamepads = 4;

    enum GamepadAxis {
        GAMEPAD_AXIS_LEFTX = 0,
        GAMEPAD_AXIS_LEFTY,
        GAMEPAD_AXIS_RIGHTX,
        GAMEPAD_AXIS_RIGHTY,
        GAMEPAD_AXIS_TRIGGERLEFT,
        GAMEPAD_AXIS_TRIGGERRIGHT,
        GAMEPAD_AXIS_COUNT
    };

    enum GamepadButtons {
        GAMEPAD_BUTTON_A = 0,
        GAMEPAD_BUTTON_B,
        GAMEPAD_BUTTON_X,
        GAMEPAD_BUTTON_Y,
        GAMEPAD_BUTTON_BACK,
        GAMEPAD_BUTTON_GUIDE,
        GAMEPAD_BUTTON_START,
        GAMEPAD_BUTTON_LEFTSTICK,
        GAMEPAD_BUTTON_RIGHTSTICK,
        GAMEPAD_BUTTON_LEFTSHOULDER,
        GAMEPAD_BUTTON_RIGHTSHOULDER,
        GAMEPAD_BUTTON_DPAD_UP,
        GAMEPAD_BUTTON_DPAD_DOWN,
        GAMEPAD_BUTTON_DPAD_LEFT,
        GAMEPAD_BUTTON_DPAD_RIGHT,
        GAMEPAD_BUTTON_COUNT
    };

    enum MouseButtons {
        MOUSE_BUTTONS_NONE = -1,
        MOUSE_BUTTONS_LEFT = 0,
        MOUSE_BUTTONS_RIGHT,
        MOUSE_BUTTONS_MIDDLE,
        MOUSE_BUTTONS_COUNT
    };

    enum Device : u8 {
        DEVICE_KEYBOARD,
        DEVICE_MOUSE,
        DEVICE_GAMEPAD
    };

    enum DevicePart : u8 {
        DEVICE_PART_KEYBOARD,
        DEVICE_PART_MOUSE,
        DEVICE_PART_GAMEPAD_AXIS,
        DEVICE_PART_GAMEPAD_BUTTONS
    };

    enum BindingType : u8 {
        BINDING_TYPE_BUTTON,
        BINDING_TYPE_AXIS_1D,
        BINDING_TYPE_AXIS_2D,
        BINDING_TYPE_VECTOR_1D,
        BINDING_TYPE_VECTOR_2D,
        BINDING_TYPE_BUTTON_ONE_MOD,
        BINDING_TYPE_BUTTON_TWO_MOD
    };


    // Utility methods ////////////////////////////////////////////////////////
    Device                              DeviceFromPart( DevicePart part );

    cstring*                            GamepadAxisNames();
    cstring*                            GamepadButtonNames();
    cstring*                            MouseButtonNames();

    struct InputVector2 {
        f32                             m_x;
        f32                             m_y;
    };

    struct Gamepad {
        f32                             m_axis[ GAMEPAD_AXIS_COUNT ];
        u8                              m_buttons[ GAMEPAD_BUTTON_COUNT ];
        u8                              m_previousButtons[ GAMEPAD_BUTTON_COUNT ];

        void*                           m_handle;
        cstring                         m_name;

        u32                             m_index;
        i32                             m_id;

        bool                            IsAttached() const                           { return m_id >= 0; }
        bool                            IsButtonDown( GamepadButtons button )        { return m_buttons[ button ]; }
        bool                            IsButtonJustPressed( GamepadButtons button ) { return ( m_buttons[ button ] && !m_previousButtons[ button ] ); }
    };

    using InputHandle = u32;

    struct InputBinding {

        BindingType                     m_type;
        Device                          m_device;
        DevicePart                      m_devicePart;
        u8                              m_actionMapIndex;

        u16                             m_actionIndex;
        u16                             m_button;         // Stores the buttons either from GAMEPAD_BUTTONS_*, KEY_*, MOUSE_BUTTON_*.

        f32                             m_value = 0.0f;

        u8                              m_isComposite;
        u8                              m_isPartOfComposite;
        u8                              m_repeat;

        f32                             m_minDeadzone    = 0.10f;
        f32                             m_maxDeadzone    = 0.95f;

        InputBinding&                   Set( BindingType type, Device device, DevicePart devicePart, u16 button, u8 isComposite, u8 isPartOfComposite, u8 repeat );
        InputBinding&                   SetDeadzones( f32 min, f32 max );
        InputBinding&                   SetHandles( InputHandle actionMap, InputHandle action );

    };

    struct InputAction {

        bool                            Triggered() const;
        f32                             ReadValue1d() const;
        InputVector2                    ReadValue2d() const;

        InputVector2                    m_value;
        InputHandle                     m_actionMap;
        cstring                         m_name;

    };

    struct InputActionMap {

        cstring                         m_name;
        bool                            m_active;

    };

    struct InputActionMapCreation {
        cstring                         m_name;
        bool                            m_active;
    };

    struct InputActionCreation {
        cstring                         m_name;
        InputHandle                     m_actionMap;
    };

    struct InputBindingCreation {
        InputHandle                     m_action;
    };

    using StringBuffer = std::basic_string<char, std::char_traits<char>, STLAdaptor<char>>;

    #define Array(Element) std::vector<Element, STLAdaptor<Element>>

    struct InputService : public Service {
        ~InputService() { Shutdown(); }
        InputService( Allocator* allocator );
        void                            Shutdown();

        bool                            IsKeyDown( Keys key );
        bool                            IsKeyJustPressed( Keys key, bool repeat = false );
        bool                            IsKeyJustReleased( Keys key );

        bool                            IsMouseDown( MouseButtons button );
        bool                            IsMouseClicked( MouseButtons button );
        bool                            IsMouseReleased( MouseButtons button );
        bool                            IsMouseDragging( MouseButtons button );

        void                            Update( f32 delta );

        void                            DebugUi();

        void                            NewFrame();            // Called before message handling
        void                            OnEvent( void* inputEvent );

        bool                            IsTriggered( InputHandle action ) const;
        f32                             IsReadValue1d( InputHandle action ) const;
        InputVector2                    IsReadValue2d( InputHandle action ) const;

        // Create methods used to create the actual input
        InputHandle                     CreateActionMap( const InputActionMapCreation& creation );
        InputHandle                     CreateAction( const InputActionCreation& creation );

        // Find methods using name
        InputHandle                     FindActionMap( cstring name ) const;
        InputHandle                     FindAction( cstring name ) const;

        void                            AddButton( InputHandle action, DevicePart device, uint16_t button, bool repeat = false );
        void                            AddAxis1d( InputHandle action, DevicePart device, uint16_t axis, float min_deadzone, float max_deadzone );
        void                            AddAxis2d( InputHandle action, DevicePart device, uint16_t x_axis, uint16_t y_axis, float min_deadzone, float max_deadzone );
        void                            AddVector1d( InputHandle action, DevicePart device_pos, uint16_t button_pos, DevicePart device_neg, uint16_t button_neg, bool repeat = true );
        void                            AddVector2d( InputHandle action, DevicePart device_up, uint16_t button_up, DevicePart device_down, uint16_t button_down, DevicePart device_left, uint16_t button_left, DevicePart device_right, uint16_t button_right, bool repeat = true );

        StringBuffer                    m_stringBuffer;

        Array(InputActionMap)           m_actionMaps;
        Array(InputAction)              m_actions;
        Array(InputBinding)             m_bindings;

        Gamepad                         m_gamepads[k_max_gamepads];

        u8                              m_keys[ KEY_COUNT ];
        u8                              m_previousKeys[ KEY_COUNT ];

        InputVector2                    m_mousePosition;
        InputVector2                    m_previousMousePosition;
        InputVector2                    m_mouseClickedPosition[ MOUSE_BUTTONS_COUNT ];
        u8                              m_mouseButton[ MOUSE_BUTTONS_COUNT ];
        u8                              m_previousMouseButton[ MOUSE_BUTTONS_COUNT ];
        f32                             m_mouseDragDistance[ MOUSE_BUTTONS_COUNT ];

        bool                            m_hasFocus;

        static InputService* Create(Allocator* allocator) {
            static std::unique_ptr<InputService> instance{new InputService(allocator)};
            return instance.get();
        }

        static constexpr cstring        m_name = "caustix_input_service";

    };
}

namespace Caustix {
    struct InputBackend {
        void Init( Gamepad* gamepads, u32 numGamepads );
        void Shutdown();

        void GetMouseState( InputVector2& position, u8* buttons, u32 numButtons );

        void OnEvent( void* event_, u8* keys, u32 numKeys, Gamepad* gamepads, u32 numGamepads, bool& hasFocus );
    };

#if defined (INPUT_BACKEND_SDL)
    static bool InitializeGamepad( int32_t index, Gamepad& gamepad ) {

        SDL_GameController* pad = SDL_GameControllerOpen( index );
        //SDL_Joystick* joy = SDL_JoystickOpen( index );

        // Set memory to 0
        memset( &gamepad, 0, sizeof( Gamepad ) );

        if ( pad ) {
            info( "Opened Joystick 0" );
            info( "Name: {}", SDL_GameControllerNameForIndex( index ) );
            //info( "Number of Axes: {}", SDL_JoystickNumAxes( joy ) );
            //info( "Number of Buttons: {}", SDL_JoystickNumButtons( joy ) );

            SDL_Joystick* joy = SDL_GameControllerGetJoystick( pad );

            gamepad.m_index = index;
            gamepad.m_name = SDL_JoystickNameForIndex( index );
            gamepad.m_handle = pad;
            gamepad.m_id = SDL_JoystickInstanceID( joy );

            return true;

        } else {
            error( "Couldn't open Joystick {}", index );
            gamepad.m_index = u32_max;

            return false;
        }
    }

    static void TerminateGamepad( Gamepad& gamepad ) {

        SDL_JoystickClose( (SDL_Joystick*)gamepad.m_handle );
        gamepad.m_index = u32_max;
        gamepad.m_name = 0;
        gamepad.m_handle = 0;
        gamepad.m_id = u32_max;
    }

// InputBackendSDL //////////////////////////////////////////////////////////////
    void InputBackend::Init( Gamepad* gamepads, u32 numGamepads ) {

        if ( SDL_WasInit( SDL_INIT_GAMECONTROLLER ) != 1 )
            SDL_InitSubSystem( SDL_INIT_GAMECONTROLLER );

        SDL_GameControllerEventState( SDL_ENABLE );

        for ( u32 i = 0; i < numGamepads; i++ ) {
            gamepads[ i ].m_index = u32_max;
            gamepads[ i ].m_id = u32_max;
        }

        const i32 num_joystics = SDL_NumJoysticks();
        if ( num_joystics > 0 ) {

            info( "Detected joysticks!" );

            for ( i32 i = 0; i < num_joystics; i++ ) {
                if ( SDL_IsGameController( i ) ) {
                    InitializeGamepad( i, gamepads[ i ] );
                }
            }
        }
    }

    void InputBackend::Shutdown() {
        SDL_GameControllerEventState( SDL_DISABLE );
    }

    static u32 ToSdlMouseButton( MouseButtons button ) {
        switch ( button ) {
            case MOUSE_BUTTONS_LEFT:
                return SDL_BUTTON_LEFT;
            case MOUSE_BUTTONS_MIDDLE:
                return SDL_BUTTON_MIDDLE;
            case MOUSE_BUTTONS_RIGHT:
                return SDL_BUTTON_RIGHT;
        }

        return u32_max;
    }

    void InputBackend::GetMouseState( InputVector2& position, u8* buttons, u32 numButtons ) {
        i32 mouse_x, mouse_y;
        u32 mouse_buttons = SDL_GetMouseState( &mouse_x, &mouse_y );

        for ( u32 i = 0; i < numButtons; ++i ) {
            u32 sdl_button = ToSdlMouseButton( ( MouseButtons )i );
            buttons[ i ] = mouse_buttons & SDL_BUTTON( sdl_button );
        }

        position.m_x = ( f32 )mouse_x;
        position.m_y = ( f32 )mouse_y;
    }

    void InputBackend::OnEvent(void *event_, u8 *keys, u32 numKeys, Caustix::Gamepad *gamepads, u32 numGamepads,
                               bool &hasFocus) {
        SDL_Event& event = *( SDL_Event* )event_;
        switch ( event.type ) {

            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                i32 key = event.key.keysym.scancode;
                if ( key >= 0 && key < (i32)numKeys )
                    keys[ key ] = ( event.type == SDL_KEYDOWN );

                break;
            }

            case SDL_CONTROLLERDEVICEADDED:
            {
                info( "Gamepad Added" );
                int32_t index = event.cdevice.which;

                InitializeGamepad( index, gamepads[ index ] );

                break;
            }

            case SDL_CONTROLLERDEVICEREMOVED:
            {
                info( "Gamepad Removed" );
                int32_t instance_id = event.jdevice.which;
                // Search for the correct gamepad
                for ( size_t i = 0; i < k_max_gamepads; i++ ) {
                    if ( gamepads[ i ].m_id == instance_id ) {
                        TerminateGamepad( gamepads[ i ] );
                        break;
                    }
                }
                break;
            }

            case SDL_CONTROLLERAXISMOTION:
            {
#if defined (INPUT_DEBUG_OUTPUT)
                info( "Axis {} - {}", event.jaxis.axis, event.jaxis.value / 32768.0f );
#endif // INPUT_DEBUG_OUTPUT

                for ( size_t i = 0; i < k_max_gamepads; i++ ) {
                    if ( gamepads[ i ].m_id == event.caxis.which ) {
                        gamepads[ i ].m_axis[ event.caxis.axis ] = event.caxis.value / 32768.0f;
                        break;
                    }
                }
                break;
            }

            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP:
            {
#if defined (INPUT_DEBUG_OUTPUT)
                info( "Button" );
#endif // INPUT_DEBUG_OUTPUT

                for ( size_t i = 0; i < k_max_gamepads; i++ ) {
                    if ( gamepads[ i ].m_id == event.cbutton.which ) {
                        gamepads[ i ].m_buttons[ event.cbutton.button ] = event.cbutton.state == SDL_PRESSED ? 1 : 0;
                        break;
                    }
                }
                break;
            }

#if 0
        case SDL_JOYDEVICEADDED:
        {
            info( "Joystick Added" );
            int32_t joystick_index = event.jdevice.which;

            InitializeGamepad( joystick_index, gamepads[ joystick_index ] );

            break;
        }

        case SDL_JOYDEVICEREMOVED:
        {
            info( "joystick Removed" );
            int32_t joystick_instance_id = event.jdevice.which;
            // Search for the correct gamepad
            for ( size_t i = 0; i < k_max_gamepads; i++ ) {
                if ( gamepads[ i ].m_id == joystick_instance_id ) {
                    TerminateGamepad( gamepads[ i ] );
                    break;
                }
            }

            break;
        }

        case SDL_JOYAXISMOTION:
        {
#if defined (INPUT_DEBUG_OUTPUT)
            info( "Axis {} - {}", event.jaxis.axis, event.jaxis.value / 32768.0f );
#endif // INPUT_DEBUG_OUTPUT

            for ( size_t i = 0; i < k_max_gamepads; i++ ) {
                if ( gamepads[ i ].m_id == event.jaxis.which ) {
                    gamepads[ i ].m_axis[ event.jaxis.axis ] = event.jaxis.value / 32768.0f;
                }
            }
            break;
        }

        case SDL_JOYBALLMOTION:
        {
#if defined (INPUT_DEBUG_OUTPUT)
            info( "Ball" );
#endif // INPUT_DEBUG_OUTPUT

            for ( size_t i = 0; i < k_max_gamepads; i++ ) {
                if ( gamepads[ i ].m_id == event.jball.which ) {
                    break;
                }
            }
            break;
        }

        case SDL_JOYHATMOTION:
        {
#if defined (INPUT_DEBUG_OUTPUT)
            info( "Hat" );
#endif // INPUT_DEBUG_OUTPUT

            /*for ( size_t i = 0; i < k_max_gamepads; i++ ) {
                if ( gamepads[ i ].id == event.jhat.which ) {
                    gamepads[ i ].hats[ event.jhat.hat ] = event.jhat.value;
                    break;
                }
            }*/
            break;
        }

        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
        {
#if defined (INPUT_DEBUG_OUTPUT)
            info( "Button" );
#endif // INPUT_DEBUG_OUTPUT

            for ( size_t i = 0; i < k_max_gamepads; i++ ) {
                if ( gamepads[ i ].m_id == event.jbutton.which ) {
                    gamepads[ i ].m_buttons[ event.jbutton.button ] = event.jbutton.state == SDL_PRESSED ? 1 : 0;
                    break;
                }
            }
            break;
        }
#endif // Disabled old SDL joystick code

            case SDL_WINDOWEVENT:
            {
                switch ( event.window.event ) {
                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                    {
                        hasFocus = true;
                        break;
                    }

                    case SDL_WINDOWEVENT_FOCUS_LOST:
                    {
                        hasFocus = false;
                        break;
                    }
                }
                break;
            }
        }
    }

#else

    // STUB implementation

void InputBackend::Initialize( Gamepad* gamepads, u32 num_gamepads ) {
}

void InputBackend::Shutdown() {
}

void InputBackend::OnEvent( void* event_, Gamepad* gamepads, u32 num_gamepads, bool& has_focus ) {
}

void InputBackend::GetMouseState( InputVector2& position, u8* buttons, u32 num_buttons ) {
    position.x = FLT_MAX;
    position.y = FLT_MAX;
}

#endif // INPUT_BACKEND_SDL

    Device DeviceFromPart( DevicePart part ) {
        switch ( part ) {
            case DEVICE_PART_MOUSE:
            {
                return DEVICE_MOUSE;
            }

            case DEVICE_PART_GAMEPAD_AXIS:
            case DEVICE_PART_GAMEPAD_BUTTONS:
                //case InputBinding::GAMEPAD_HAT:
            {
                return DEVICE_GAMEPAD;
            }

            case DEVICE_PART_KEYBOARD:
            default:
            {
                return DEVICE_KEYBOARD;
            }
        }
    }


    cstring* GamepadAxisNames() {
        static cstring names[] = { "left_x", "left_y", "right_x", "right_y", "trigger_left", "trigger_right", "gamepad_axis_count" };
        return names;
    }

    cstring* GamepadButtonNames() {
        static cstring names[] = {"a", "b", "x", "y", "back", "guide", "start", "left_stick", "right_stick", "left_shoulder", "right_shoulder", "dpad_up", "dpad_down", "dpad_left", "dpad_right", "gamepad_button_count", };
        return names;
    }

    cstring* MouseButtonNames() {
        static cstring names[] = { "left", "right", "middle", "mouse_button_count", };
        return names;
    }



// InputService //////////////////////////////////////////////////////////////////
    static InputBackend s_input_backend;

    void InputService::Shutdown() {

        s_input_backend.Shutdown();
        m_actionMaps.clear();
        m_actions.clear();
        m_bindings.clear();

        m_stringBuffer.clear();

        info( "InputService shutdown" );
    }

    static constexpr f32 k_mouse_drag_min_distance = 4.f;

    bool InputService::IsKeyDown( Keys key ) {
        return m_keys[ key ] && m_hasFocus;
    }

    bool InputService::IsKeyJustPressed( Keys key, bool repeat ) {
        return m_keys[ key ] && !m_previousKeys[ key ] && m_hasFocus;
    }

    bool InputService::IsKeyJustReleased( Keys key ) {
        return !m_keys[ key ] && m_previousKeys[ key ] && m_hasFocus;
    }

    bool InputService::IsMouseDown( MouseButtons button ) {
        return m_mouseButton[ button ];
    }

    bool InputService::IsMouseClicked( MouseButtons button ) {
        return m_mouseButton[ button ] && !m_previousMouseButton[ button ];
    }

    bool InputService::IsMouseReleased( MouseButtons button ) {
        return !m_mouseButton[ button ];
    }

    bool InputService::IsMouseDragging( MouseButtons button ) {
        if ( !m_mouseButton[ button ] )
            return false;

        return m_mouseDragDistance[ button ] > k_mouse_drag_min_distance;
    }

    void InputService::OnEvent(void *inputEvent) {
        s_input_backend.OnEvent( inputEvent, m_keys, KEY_COUNT, m_gamepads, k_max_gamepads, m_hasFocus );
    }

    bool InputService::IsTriggered( InputHandle action ) const {
        CASSERT( action < m_actions.size() );
        return m_actions[ action ].Triggered();
    }

    f32 InputService::IsReadValue1d( InputHandle action ) const {
        CASSERT( action < m_actions.size() );
        return m_actions[ action ].ReadValue1d();
    }

    InputVector2 InputService::IsReadValue2d( InputHandle action ) const {
        CASSERT( action < m_actions.size() );
        return m_actions[ action ].ReadValue2d();
    }

    InputHandle InputService::CreateActionMap( const InputActionMapCreation& creation ) {
        InputActionMap new_m_actionMap;
        new_m_actionMap.m_active = creation.m_active;
        new_m_actionMap.m_name = creation.m_name;

        m_actionMaps.push_back( new_m_actionMap );

        return m_actionMaps.size() - 1;
    }

    InputHandle InputService::CreateAction( const InputActionCreation& creation ) {
        InputAction action;
        action.m_actionMap = creation.m_actionMap;
        action.m_name = creation.m_name;

        m_actions.push_back( action );

        return m_actions.size() - 1;
    }

    InputHandle InputService::FindActionMap( cstring name ) const {
        // TODO: move to hash map ?
        for ( u32 i = 0; i < m_actionMaps.size(); i++ ) 		{
            //info( "{}, {}", name, m_actionMaps[ i ].m_name );
            if ( strcmp( name, m_actionMaps[ i ].m_name ) == 0 ) {
                return i;
            }
        }
        return InputHandle(u32_max);
    }

    InputHandle InputService::FindAction( cstring name ) const {
        // TODO: move to hash map ?
        for ( u32 i = 0; i < m_actions.size(); i++ ) {
            //rprint( "%s, %s", name, actions[ i ].name );
            if ( strcmp( name, m_actions[ i ].m_name ) == 0 ) {
                return i;
            }
        }
        return InputHandle( u32_max );
    }

    void InputService::AddButton( InputHandle action, DevicePart device_part, uint16_t button, bool repeat ) {
        const InputAction& binding_action = m_actions[ action ];

        InputBinding binding;
        binding.Set( BINDING_TYPE_BUTTON, DeviceFromPart( device_part ), device_part, button, 0, 0, repeat ).SetHandles( binding_action.m_actionMap, action );

        m_bindings.push_back( binding );
    }

    void InputService::AddAxis1d( InputHandle action, DevicePart device_part, uint16_t axis, float min_deadzone, float max_deadzone ) {
        const InputAction& binding_action = m_actions[ action ];

        InputBinding binding;
        binding.Set( BINDING_TYPE_AXIS_1D, DeviceFromPart( device_part ), device_part, axis, 0, 0, 0 ).SetDeadzones( min_deadzone, max_deadzone ).SetHandles( binding_action.m_actionMap, action );

        m_bindings.push_back( binding );
    }

    void InputService::AddAxis2d( InputHandle action, DevicePart device_part, uint16_t x_axis, uint16_t y_axis, float min_deadzone, float max_deadzone ) {
        const InputAction& binding_action = m_actions[ action ];

        InputBinding binding, binding_x, binding_y;
        binding.Set( BINDING_TYPE_AXIS_2D, DeviceFromPart( device_part ), device_part, u16_max, 1, 0, 0 ).SetDeadzones( min_deadzone, max_deadzone ).SetHandles( binding_action.m_actionMap, action );
        binding_x.Set( BINDING_TYPE_AXIS_2D, DeviceFromPart( device_part ), device_part, x_axis, 0, 1, 0 ).SetDeadzones( min_deadzone, max_deadzone ).SetHandles( binding_action.m_actionMap, action );
        binding_y.Set( BINDING_TYPE_AXIS_2D, DeviceFromPart( device_part ), device_part, y_axis, 0, 1, 0 ).SetDeadzones( min_deadzone, max_deadzone ).SetHandles( binding_action.m_actionMap, action );

        m_bindings.push_back( binding );
        m_bindings.push_back( binding_x );
        m_bindings.push_back( binding_y );
    }

    void InputService::AddVector1d( InputHandle action, DevicePart device_part_pos, uint16_t button_pos, DevicePart device_part_neg, uint16_t button_neg, bool repeat ) {
        const InputAction& binding_action = m_actions[ action ];

        InputBinding binding, binding_positive, binding_negative;
        binding.Set( BINDING_TYPE_VECTOR_1D, DeviceFromPart( device_part_pos ), device_part_pos, u16_max, 1, 0, repeat ).SetHandles( binding_action.m_actionMap, action );
        binding_positive.Set( BINDING_TYPE_VECTOR_1D, DeviceFromPart( device_part_pos ), device_part_pos, button_pos, 0, 1, repeat ).SetHandles( binding_action.m_actionMap, action );
        binding_negative.Set( BINDING_TYPE_VECTOR_1D, DeviceFromPart( device_part_neg ), device_part_neg, button_neg, 0, 1, repeat ).SetHandles( binding_action.m_actionMap, action );

        m_bindings.push_back( binding );
        m_bindings.push_back( binding_positive );
        m_bindings.push_back( binding_negative );
    }

    void InputService::AddVector2d( InputHandle action, DevicePart device_part_up, uint16_t button_up, DevicePart device_part_down, uint16_t button_down, DevicePart device_part_left, uint16_t button_left, DevicePart device_part_right, uint16_t button_right, bool repeat ) {
        const InputAction& binding_action = m_actions[ action ];

        InputBinding binding, binding_up, binding_down, binding_left, binding_right;

        binding.Set( BINDING_TYPE_VECTOR_2D, DeviceFromPart( device_part_up ), device_part_up, u16_max, 1, 0, repeat ).SetHandles( binding_action.m_actionMap, action );
        binding_up.Set( BINDING_TYPE_VECTOR_2D, DeviceFromPart( device_part_up ), device_part_up, button_up, 0, 1, repeat ).SetHandles( binding_action.m_actionMap, action );
        binding_down.Set( BINDING_TYPE_VECTOR_2D, DeviceFromPart( device_part_down ), device_part_down, button_down, 0, 1, repeat ).SetHandles( binding_action.m_actionMap, action );
        binding_left.Set( BINDING_TYPE_VECTOR_2D, DeviceFromPart( device_part_left ), device_part_left, button_left, 0, 1, repeat ).SetHandles( binding_action.m_actionMap, action );
        binding_right.Set( BINDING_TYPE_VECTOR_2D, DeviceFromPart( device_part_right ), device_part_right, button_right, 0, 1, repeat ).SetHandles( binding_action.m_actionMap, action );

        m_bindings.push_back( binding );
        m_bindings.push_back( binding_up );
        m_bindings.push_back( binding_down );
        m_bindings.push_back( binding_left );
        m_bindings.push_back( binding_right );

    }

    void InputService::NewFrame() {
        // Cache previous frame keys.
        // Resetting previous frame breaks key pressing - there can be more frames between key presses even if continuously pressed.
        for ( u32 i = 0; i < KEY_COUNT; ++i ) {
            m_previousKeys[ i ] = m_keys[ i ];
            //keys[ i ] = 0;
        }

        for ( u32 i = 0; i < MOUSE_BUTTONS_COUNT; ++i ) {
            m_previousMouseButton[ i ] = m_mouseButton[ i ];
        }

        for ( u32 i = 0; i < k_max_gamepads; ++i ) {
            if ( m_gamepads[ i ].IsAttached() ) {
                for ( u32 k = 0; k < GAMEPAD_BUTTON_COUNT; k++ ) {
                    m_gamepads[ i ].m_previousButtons[ k ] = m_gamepads[ i ].m_buttons[ k ];
                }
            }
        }
    }

    void InputService::Update( f32 delta ) {

        // Update Mouse ////////////////////////////////////////
        m_previousMousePosition = m_mousePosition;
        // Update current mouse state
        s_input_backend.GetMouseState( m_mousePosition, m_mouseButton, MOUSE_BUTTONS_COUNT );

        for ( u32 i = 0; i < MOUSE_BUTTONS_COUNT; ++i ) {
            // Just clicked. Save position
            if ( IsMouseClicked( ( MouseButtons )i ) ) {
                m_mouseClickedPosition[ i ] = m_mousePosition;
            }
            else if ( IsMouseDown( ( MouseButtons )i ) ) {
                f32 delta_x = m_mousePosition.m_x - m_mouseClickedPosition[ i ].m_x;
                f32 delta_y = m_mousePosition.m_y - m_mouseClickedPosition[ i ].m_y;
                m_mouseDragDistance[ i ] = sqrtf( (delta_x * delta_x) + (delta_y * delta_y) );
            }
        }

        // NEW UPDATE

        // Update all actions maps
        // Update all actions
        // Scan each action of the map
        for ( u32 j = 0; j < m_actions.size(); j++ ) {
            InputAction& input_action = m_actions[ j ];
            input_action.m_value = { 0,0 };
        }

        // Read all input values for each binding
        // First get all the button or composite parts. Composite input will be calculated after.
        for ( u32 k = 0; k < m_bindings.size(); k++ ) {
            InputBinding& input_binding = m_bindings[ k ];
            // Skip composite bindings. Their value will be calculated after.
            if ( input_binding.m_isComposite )
                continue;

            input_binding.m_value = false;

            switch ( input_binding.m_device ) {
                case DEVICE_KEYBOARD:
                {
                    bool key_value = input_binding.m_repeat ? IsKeyDown( ( Keys )input_binding.m_button ) : IsKeyJustPressed( ( Keys )input_binding.m_button, false );
                    input_binding.m_value = key_value ? 1.0f : 0.0f;
                    break;
                }

                case DEVICE_GAMEPAD:
                {
                    Gamepad& gamepad = m_gamepads[ 0 ];
                    if ( gamepad.m_handle == nullptr ) {
                        break;
                    }

                    const float min_deadzone = input_binding.m_minDeadzone;
                    const float max_deadzone = input_binding.m_maxDeadzone;

                    switch ( input_binding.m_devicePart ) {
                        case DEVICE_PART_GAMEPAD_AXIS:
                        {
                            input_binding.m_value = gamepad.m_axis[ input_binding.m_button ];
                            input_binding.m_value = fabs( input_binding.m_value ) < min_deadzone ? 0.0f : input_binding.m_value;
                            input_binding.m_value = fabs( input_binding.m_value ) > max_deadzone ? ( input_binding.m_value < 0 ? -1.0f : 1.0f ) : input_binding.m_value;

                            break;
                        }
                        case DEVICE_PART_GAMEPAD_BUTTONS:
                        {
                            //input_binding.value = gamepad.buttons[ input_binding.button ];
                            input_binding.m_value = input_binding.m_repeat ? gamepad.IsButtonDown( ( GamepadButtons )input_binding.m_button ) : gamepad.IsButtonJustPressed( ( GamepadButtons )input_binding.m_button );
                            break;
                        }
                            /*case InputBinding::GAMEPAD_HAT:
                            {
                                input_binding.value = gamepad.hats[ input_binding.button ];
                                break;
                            }*/

                    }
                }
            }
        }

        for ( u32 k = 0; k < m_bindings.size(); k++ ) {
            InputBinding& input_binding = m_bindings[ k ];

            if ( input_binding.m_isPartOfComposite )
                continue;

            InputAction& input_action = m_actions[ input_binding.m_actionIndex ];

            switch ( input_binding.m_type ) {
                case BINDING_TYPE_BUTTON:
                {
                    input_action.m_value.m_x = fmax( input_action.m_value.m_x, input_binding.m_value ? 1.0f : 0.0f );
                    break;
                }

                case BINDING_TYPE_AXIS_1D:
                {
                    input_action.m_value.m_x = input_binding.m_value != 0.f ? input_binding.m_value : input_action.m_value.m_x;
                    break;
                }

                case BINDING_TYPE_AXIS_2D:
                {
                    // Retrieve following 2 bindings
                    InputBinding& input_binding_x = m_bindings[ ++k ];
                    InputBinding& input_binding_y = m_bindings[ ++k ];

                    input_action.m_value.m_x = input_binding_x.m_value != 0.0f ? input_binding_x.m_value : input_action.m_value.m_x;
                    input_action.m_value.m_y = input_binding_y.m_value != 0.0f ? input_binding_y.m_value : input_action.m_value.m_y;

                    break;
                }

                case BINDING_TYPE_VECTOR_1D:
                {
                    // Retrieve following 2 bindings
                    InputBinding& input_binding_pos = m_bindings[ ++k ];
                    InputBinding& input_binding_neg = m_bindings[ ++k ];

                    input_action.m_value.m_x = input_binding_pos.m_value ? input_binding_pos.m_value : input_binding_neg.m_value ? -input_binding_neg.m_value : input_action.m_value.m_x;
                    break;
                }

                case BINDING_TYPE_VECTOR_2D:
                {
                    // Retrieve following 4 bindings
                    InputBinding& input_binding_up = m_bindings[ ++k ];
                    InputBinding& input_binding_down = m_bindings[ ++k ];
                    InputBinding& input_binding_left = m_bindings[ ++k ];
                    InputBinding& input_binding_right = m_bindings[ ++k ];

                    input_action.m_value.m_x = input_binding_right.m_value ? 1.0f : input_binding_left.m_value ? -1.0f : input_action.m_value.m_x;
                    input_action.m_value.m_y = input_binding_up.m_value ? 1.0f : input_binding_down.m_value ? -1.0f : input_action.m_value.m_y;
                    break;
                }
            }
        }


        // Update all Input Actions ////////////////////////////
        // TODO: flat all arrays
        // Scan each action map
        /*
        for ( u32 i = 0; i < input_m_actionMaps.size; i++ ) {
            InputActionMap& m_actionMap = input_m_actionMaps[ i ];
            if ( !m_actionMap.active ) {
                continue;
            }

            // Scan each action of the map
            for ( u32 j = 0; j < m_actionMap.num_actions; j++ ) {
                InputAction& input_action = m_actionMap.actions[ j ];

                // First get all the button or composite parts. Composite input will be calculated after.
                for ( u32 k = 0; k < input_action.bindings.size; k++ ) {
                    InputBinding& input_binding = input_action.bindings[ k ];
                    // Skip composite bindings. Their value will be calculated after.
                    if ( input_binding.is_composite )
                        continue;

                    input_binding.value = false;

                    switch ( input_binding.device ) {
                        case DEVICE_KEYBOARD:
                        {
                            bool key_value = input_binding.repeat ? is_key_down( (Keys)input_binding.button ) : is_key_just_pressed( (Keys)input_binding.button, false );
                            input_binding.value = key_value ? 1.0f : 0.0f;
                            break;
                        }

                        case DEVICE_GAMEPAD:
                        {
                            Gamepad& gamepad = gamepads[ 0 ];
                            if ( gamepad.handle == nullptr ) {
                                break;
                            }

                            const float min_deadzone = input_binding.min_deadzone;
                            const float max_deadzone = input_binding.max_deadzone;

                            switch ( input_binding.device_part ) {
                                case GAMEPAD_AXIS:
                                {
                                    input_binding.value = gamepad.axis[ input_binding.button ];
                                    input_binding.value = fabs( input_binding.value ) < min_deadzone ? 0.0f : input_binding.value;
                                    input_binding.value = fabs( input_binding.value ) > max_deadzone ? ( input_binding.value < 0 ? -1.0f : 1.0f ) : input_binding.value;

                                    break;
                                }
                                case GAMEPAD_BUTTONS:
                                {
                                    input_binding.value = gamepad.buttons[ input_binding.button ];
                                    break;
                                }
                                /*case InputBinding::GAMEPAD_HAT:
                                {
                                    input_binding.value = gamepad.hats[ input_binding.button ];
                                    break;
                                }* /
                            }

                            break;
                        }
                    }
                }

                // Calculate/syntethize composite input values into input action
                input_action.value = { 0,0 };

                for ( u32 k = 0; k < input_action.bindings.size; k++ ) {
                    InputBinding& input_binding = input_action.bindings[ k ];

                    if ( input_binding.is_part_of_composite )
                        continue;

                    switch ( input_binding.type ) {
                        case BUTTON:
                        {
                            input_action.value.x = fmax( input_action.value.x, input_binding.value ? 1.0f : 0.0f );
                            break;
                        }

                        case AXIS_1D:
                        {
                            input_action.value.x = input_binding.value != 0.f ? input_binding.value : input_action.value.x;
                            break;
                        }

                        case AXIS_2D:
                        {
                            // Retrieve following 2 bindings
                            InputBinding& input_binding_x = input_action.bindings[ ++k ];
                            InputBinding& input_binding_y = input_action.bindings[ ++k ];

                            input_action.value.x = input_binding_x.value != 0.0f ? input_binding_x.value : input_action.value.x;
                            input_action.value.y = input_binding_y.value != 0.0f ? input_binding_y.value : input_action.value.y;

                            break;
                        }

                        case VECTOR_1D:
                        {
                            // Retrieve following 2 bindings
                            InputBinding& input_binding_pos = input_action.bindings[ ++k ];
                            InputBinding& input_binding_neg = input_action.bindings[ ++k ];

                            input_action.value.x = input_binding_pos.value ? input_binding_pos.value : input_binding_neg.value ? -input_binding_neg.value : input_action.value.x;
                            break;
                        }

                        case VECTOR_2D:
                        {
                            // Retrieve following 4 bindings
                            InputBinding& input_binding_up = input_action.bindings[ ++k ];
                            InputBinding& input_binding_down = input_action.bindings[ ++k ];
                            InputBinding& input_binding_left = input_action.bindings[ ++k ];
                            InputBinding& input_binding_right = input_action.bindings[ ++k ];

                            input_action.value.x = input_binding_right.value ? 1.0f : input_binding_left.value ? -1.0f : input_action.value.x;
                            input_action.value.y = input_binding_up.value ? 1.0f : input_binding_down.value ? -1.0f : input_action.value.y;
                            break;
                        }
                    }
                }
            }
        }
        */
    }

    void InputService::DebugUi() {
        if ( ImGui::Begin( "Input" ) ) {
            ImGui::Text( "Has focus %u", m_hasFocus ? 1 : 0 );

            if ( ImGui::TreeNode( "Devices" ) ) {
                ImGui::Separator();
                if ( ImGui::TreeNode( "Gamepads" ) ) {
                    for ( u32 i = 0; i < k_max_gamepads; ++i ) {
                        const Gamepad& g = m_gamepads[ i ];
                        ImGui::Text( "Name: %s, id %d, index %u", g.m_name, g.m_id, g.m_index );
                        // Attached gamepad
                        if ( g.IsAttached() ) {
                            ImGui::NewLine();
                            ImGui::Columns( GAMEPAD_AXIS_COUNT );
                            for ( u32 gi = 0; gi < GAMEPAD_AXIS_COUNT; gi++ ) {
                                ImGui::Text( "%s", GamepadAxisNames()[gi] );
                                ImGui::NextColumn();
                            }
                            for ( u32 gi = 0; gi < GAMEPAD_AXIS_COUNT; gi++ ) {
                                ImGui::Text( "%f", g.m_axis[ gi ] );
                                ImGui::NextColumn();
                            }
                            ImGui::NewLine();
                            ImGui::Columns( GAMEPAD_BUTTON_COUNT );
                            for ( u32 gi = 0; gi < GAMEPAD_BUTTON_COUNT; gi++ ) {
                                ImGui::Text( "%s", GamepadButtonNames()[ gi ] );
                                ImGui::NextColumn();
                            }
                            ImGui::Columns( GAMEPAD_BUTTON_COUNT );
                            for ( u32 gi = 0; gi < GAMEPAD_BUTTON_COUNT; gi++ ) {
                                ImGui::Text( "%u", g.m_buttons[ gi ] );
                                ImGui::NextColumn();
                            }

                            ImGui::Columns( 1 );
                        }
                        ImGui::Separator();
                    }
                    ImGui::TreePop();
                }

                ImGui::Separator();
                if ( ImGui::TreeNode( "Mouse" ) ) {
                    ImGui::Text( "Position     %f,%f", m_mousePosition.m_x, m_mousePosition.m_y );
                    ImGui::Text( "Previous pos %f,%f", m_previousMousePosition.m_x, m_previousMousePosition.m_y );

                    ImGui::Separator();

                    for ( u32 i = 0; i < MOUSE_BUTTONS_COUNT; i++ ) {
                        ImGui::Text( "Button %u", i );
                        ImGui::SameLine();
                        ImGui::Text( "Clicked Position     %4.1f,%4.1f", m_mouseClickedPosition[i].m_x, m_mouseClickedPosition[i].m_y );
                        ImGui::SameLine();
                        ImGui::Text( "Button %u, Previous %u", m_mouseButton[i], m_previousMouseButton[i] );
                        ImGui::SameLine();
                        ImGui::Text( "Drag %f", m_mouseDragDistance[i] );

                        ImGui::Separator();
                    }
                    ImGui::TreePop();
                }

                ImGui::Separator();
                if ( ImGui::TreeNode( "Keyboard" ) ) {
                    for ( u32 i = 0; i < KEY_LAST; i++ ) {

                    }
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }

            if ( ImGui::TreeNode( "Actions" ) ) {

                for ( u32 j = 0; j < m_actions.size(); j++ ) {
                    const InputAction& input_action = m_actions[ j ];
                    ImGui::Text( "Action %s, x %2.3f y %2.3f", input_action.m_name, input_action.m_value.m_x, input_action.m_value.m_y );
                }

                ImGui::TreePop();
            }

            if ( ImGui::TreeNode( "Bindings" ) ) {
                for ( u32 k = 0; k < m_bindings.size(); k++ ) {
                    const InputBinding& binding = m_bindings[ k ];
                    const InputAction& parent_action = m_actions[ binding.m_actionIndex ];

                    cstring button_name = "";
                    switch ( binding.m_devicePart ) {
                        case DEVICE_PART_KEYBOARD:
                        {
                            button_name = KeyNames()[ binding.m_button ];
                            break;
                        }
                        case DEVICE_PART_MOUSE:
                        {
                            break;
                        }
                        case DEVICE_PART_GAMEPAD_AXIS:
                        {
                            break;
                        }
                        case DEVICE_PART_GAMEPAD_BUTTONS:
                        {
                            break;
                        }
                    }

                    switch ( binding.m_type ) {
                        case BINDING_TYPE_VECTOR_1D:
                        {
                            ImGui::Text( "Binding action %s, type %s, value %f, composite %u, part of composite %u, button %s", parent_action.m_name, "vector 1d", binding.m_value, binding.m_isComposite, binding.m_isPartOfComposite, button_name );
                            break;
                        }
                        case BINDING_TYPE_VECTOR_2D:
                        {
                            ImGui::Text( "Binding action %s, type %s, value %f, composite %u, part of composite %u", parent_action.m_name, "vector 2d", binding.m_value, binding.m_isComposite, binding.m_isPartOfComposite );
                            break;
                        }
                        case BINDING_TYPE_AXIS_1D:
                        {
                            ImGui::Text( "Binding action %s, type %s, value %f, composite %u, part of composite %u", parent_action.m_name, "axis 1d", binding.m_value, binding.m_isComposite, binding.m_isPartOfComposite );
                            break;
                        }
                        case BINDING_TYPE_AXIS_2D:
                        {
                            ImGui::Text( "Binding action %s, type %s, value %f, composite %u, part of composite %u", parent_action.m_name, "axis 2d", binding.m_value, binding.m_isComposite, binding.m_isPartOfComposite );
                            break;
                        }
                        case BINDING_TYPE_BUTTON:
                        {
                            ImGui::Text( "Binding action %s, type %s, value %f, composite %u, part of composite %u, button %s", parent_action.m_name, "button", binding.m_value, binding.m_isComposite, binding.m_isPartOfComposite, button_name );
                            break;
                        }
                    }
                }

                ImGui::TreePop();
            }

        }
        ImGui::End();
    }

// InputAction /////////////////////////////////////////////////////////

    bool InputAction::Triggered() const {
        return m_value.m_x != 0.0f;
    }

    float InputAction::ReadValue1d() const {
        return m_value.m_x;
    }

    InputVector2 InputAction::ReadValue2d() const {
        return m_value;
    }


    InputBinding& InputBinding::Set( BindingType type_, Device device_, DevicePart device_part_, u16 button_, u8 is_composite_, u8 is_part_of_composite_, u8 repeat_ ) {
        m_type = type_;
        m_device = device_;
        m_devicePart = device_part_;
        m_button = button_;
        m_isComposite = is_composite_;
        m_isPartOfComposite = is_part_of_composite_;
        m_repeat = repeat_;
        return *this;
    }

    InputBinding& InputBinding::SetDeadzones( f32 min, f32 max ) {
        m_minDeadzone = min;
        m_maxDeadzone = max;
        return *this;
    }

    InputBinding& InputBinding::SetHandles( InputHandle m_actionMap, InputHandle action ) {
        // Don't expect this to have more than 256.
        CASSERT( m_actionMap < 256 );
        CASSERT( action < 16636 );

        m_actionMapIndex = ( u8 )m_actionMap;
        m_actionIndex = ( u16 )action;

        return *this;
    }

    InputService::InputService(Allocator *allocator)
    : m_stringBuffer(*allocator)
    , m_actionMaps(*allocator)
    , m_actions(*allocator)
    , m_bindings(*allocator)
    {
        info( "InputService init" );

        m_stringBuffer.reserve( 1000 );
        m_actionMaps.reserve( 16 );
        m_actions.reserve( 64 );
        m_bindings.reserve( 256 );

        // Init gamepads handles
        for ( size_t i = 0; i < k_max_gamepads; i++ ) {
            m_gamepads[ i ].m_handle = nullptr;
        }
        memset( m_keys, 0, KEY_COUNT );
        memset( m_previousKeys, 0, KEY_COUNT );
        memset( m_mouseButton, 0, MOUSE_BUTTONS_COUNT );
        memset( m_previousMouseButton, 0, MOUSE_BUTTONS_COUNT );

        s_input_backend.Init( m_gamepads, k_max_gamepads );
    }
}