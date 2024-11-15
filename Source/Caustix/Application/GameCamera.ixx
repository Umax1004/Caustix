module;

#include <corecrt_math.h>
#include <imgui.h>
#include <cglm/types-struct.h>
#include <cglm/util.h>
#include <cglm/struct/affine.h>
#include <cglm/struct/vec3.h>

#include <cmath>

export module Application.GameCamera;

import Application.Input;

import Foundation.Camera;
import Foundation.Platform;
import Foundation.Numerics;
import Foundation.Log;

export namespace Caustix {
    struct GameCamera {
        explicit GameCamera( bool enabled = true, f32 rotationSpeed = 1.0f, f32 movementSpeed = 10.f, f32 movementDelta = 0.1f);
        void Reset();

        void Update(InputService* input, u32 windowWidth, u32 windowHeight, f32 deltaTime);
        void ApplyJittering(f32 x, f32 y);

        Camera  m_camera;

        f32     m_targetYaw;
        f32     m_targetPitch;

        f32     m_mouseSensitivity;
        f32     m_movementDelta;
        u32     m_ignoreDraggingFrames;

        vec3s   m_targetMovement;

        bool    m_enabled;
        bool    m_mouseDragging;

        f32     m_rotationSpeed;
        f32     m_movementSpeed;
    };
}

namespace Caustix {
    GameCamera::GameCamera(bool enabled, f32 rotationSpeed, f32 movementSpeed, f32 movementDelta)
        : m_enabled(enabled)
        , m_rotationSpeed(rotationSpeed)
        , m_movementSpeed(movementSpeed)
        , m_movementDelta(movementDelta)
    {
        Reset();
    }

    void GameCamera::Reset() {
        m_targetYaw = 0.0f;
        m_targetPitch = 0.0f;

        m_targetMovement = m_camera.m_position;

        m_mouseDragging = false;
        m_ignoreDraggingFrames = 3;
        m_mouseSensitivity = 1.0f;
    }

    // Taken from this article:
    // http://www.rorydriscoll.com/2016/03/07/frame-rate-independent-damping-using-lerp/
    //
    float lerp( float a, float b, float t, float dt ) {
        return glm_lerp( a, b, 1.f - powf( 1 - t, dt ) );
    }

    vec3s lerp3( const vec3s& from, const vec3s& to, f32 t, f32 dt ) {
        return vec3s{ lerp(from.x, to.x, t, dt), lerp( from.y, to.y, t, dt ), lerp( from.z, to.z, t, dt ) };
    }

    void GameCamera::Update(InputService* input, u32 windowWidth, u32 windowHeight, f32 deltaTime) {
        if (!m_enabled) {
            return;
        }

        m_camera.Update();

        // Ignore first dragging frames for mouse movement waiting the cursor to be placed at the center of the screen.

        if (input->IsMouseDragging(MOUSE_BUTTONS_RIGHT) && !ImGui::IsAnyItemHovered()) {
            if (m_ignoreDraggingFrames == 0) {
                m_targetYaw -= (input->m_mousePosition.m_x -  input->m_previousMousePosition.m_x) * m_mouseSensitivity * deltaTime;
                m_targetPitch -= (input->m_mousePosition.m_y - input->m_previousMousePosition.m_y) * m_mouseSensitivity * deltaTime;
                m_targetPitch = clamp( m_targetPitch, -std::abs(m_camera.m_fovY), std::abs(m_camera.m_fovY) );

                if ( m_targetYaw > 360.0f ) {
                    m_targetYaw -= 360.0f;
                }
            }
            else {
                --m_ignoreDraggingFrames;
            }
            m_mouseDragging = true;
        }
        else {
            m_mouseDragging = false;

            m_ignoreDraggingFrames = 3;
        }

        vec3s cameraMovement {0,0,0};
        float cameraMovementDelta = m_movementDelta;

        if (input->IsKeyDown(KEY_RSHIFT) || input->IsKeyDown(KEY_LSHIFT)) {
            cameraMovementDelta *= 10.0f;
        }

        if (input->IsKeyDown(KEY_RALT) || input->IsKeyDown(KEY_LALT)) {
            cameraMovementDelta *= 100.0f;
        }

        if (input->IsKeyDown(KEY_RCTRL) || input->IsKeyDown(KEY_LCTRL)) {
            cameraMovementDelta *= 0.1f;
        }

        if (input->IsKeyDown(KEY_LEFT) || input->IsKeyDown(KEY_A)) {
            cameraMovement = glms_vec3_add( cameraMovement, glms_vec3_scale( m_camera.m_right, -cameraMovementDelta));
        }
        else if (input->IsKeyDown(KEY_RIGHT) || input->IsKeyDown(KEY_D)) {
            cameraMovement = glms_vec3_add( cameraMovement, glms_vec3_scale( m_camera.m_right, cameraMovementDelta));
        }

        if (input->IsKeyDown(KEY_PAGEDOWN) || input->IsKeyDown(KEY_E)) {
            cameraMovement = glms_vec3_add( cameraMovement, glms_vec3_scale( m_camera.m_up, -cameraMovementDelta));
        }
        else if (input->IsKeyDown(KEY_PAGEUP) || input->IsKeyDown(KEY_Q)) {
            cameraMovement = glms_vec3_add( cameraMovement, glms_vec3_scale( m_camera.m_up, cameraMovementDelta));
        }

        if (input->IsKeyDown(KEY_UP) || input->IsKeyDown(KEY_W)) {
            cameraMovement = glms_vec3_add( cameraMovement, glms_vec3_scale( m_camera.m_direction, -cameraMovementDelta));
        }
        else if (input->IsKeyDown(KEY_DOWN) || input->IsKeyDown(KEY_S)) {
            cameraMovement = glms_vec3_add( cameraMovement, glms_vec3_scale( m_camera.m_direction, cameraMovementDelta));
        }

        m_targetMovement = glms_vec3_add((vec3s&) m_targetMovement, cameraMovement);


        if (std::abs(input->m_mousePosition.m_y - input->m_previousMousePosition.m_y) < 1e-3)
        {
            m_targetPitch = 0;
        }

        if (std::abs(input->m_mousePosition.m_x -  input->m_previousMousePosition.m_x) < 1e-3)
        {
            m_targetYaw = 0;
        }

        if(m_mouseDragging)
        {
            const f32 tweenSpeed = m_rotationSpeed * deltaTime;
            m_camera.Rotate(m_targetPitch * tweenSpeed, m_targetYaw * tweenSpeed);
        }

        const f32 tweenPositionSpeed = m_movementSpeed * deltaTime;
        m_camera.m_position = lerp3(m_camera.m_position, m_targetMovement, 0.9f, tweenPositionSpeed);

                                            // info("{} - {}", m_targetPitch, m_camera.m_pitch);
    }

    void GameCamera::ApplyJittering(f32 x, f32 y) {
        // Reset camera projection
        m_camera.CalculateProjectionMatrix();

        const mat4s jitteringMatrix = glms_translate_make( { x, y, 0.0f } );
        m_camera.m_projection = glms_mat4_mul( jitteringMatrix, m_camera.m_projection );
        m_camera.CalculateViewProjection();
    }
}