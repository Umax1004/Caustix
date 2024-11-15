module;

#include "cglm/struct/mat4.h"
#include "cglm/struct/cam.h"
#include "cglm/struct/affine.h"
#include "cglm/struct/quat.h"
#include "cglm/struct/project.h"

export module Foundation.Camera;

import Foundation.Platform;

export namespace Caustix {
    struct Camera {
        void IntializePerspective(f32 nearPlane, f32 farPlane, f32 fovY, f32 aspectRatio);

        void IntializeOrthographic(f32 nearPlane, f32 farPlane, f32 viewportWidth, f32 viewportHeight, f32 zoom);

        void Reset();

        void SetViewportSize(f32 width, f32 height);

        void SetZoom(f32 zoom);

        void SetAspectRatio(f32 aspectRatio);

        void SetFovY(f32 fovY);

        void Update();

        void Rotate(f32 deltaPitch, f32 deltaYaw);

        void CalculateProjectionMatrix();

        void CalculateViewProjection();

        // Project/unproject
        [[maybe_unused]] vec3s Unproject(const vec3s &screenCoordinates);

        // Unproject by inverting the y of the screen coordinate.
        vec3s UnprojectInvertedY(const vec3s &screenCoordinates);

        void GetProjectionOrtho2D(mat4 &outMatrix);

        static void YawPitchFromDirection(const vec3s &direction, f32 &yaw, f32 &pitch);

        mat4s m_view;
        mat4s m_projection;
        mat4s m_viewProjection;

        vec3s m_position;
        vec3s m_right;
        vec3s m_direction;
        vec3s m_up;

        f32 m_yaw;
        f32 m_pitch;

        f32 m_nearPlane;
        f32 m_farPlane;

        f32 m_fovY;
        f32 m_aspectRatio;

        f32 m_zoom;
        f32 m_viewportWidth;
        f32 m_viewportHeight;

        bool m_perspective;
        bool m_updateProjection;
    };
}

namespace Caustix {
    void Camera::IntializePerspective(f32 nearPlane, f32 farPlane, f32 fovY, f32 aspectRatio) {
        m_perspective = true;

        m_nearPlane = nearPlane;
        m_farPlane = farPlane;
        m_fovY = fovY;
        m_aspectRatio = aspectRatio;

        Reset();
    }

    void Camera::IntializeOrthographic(f32 nearPlane, f32 farPlane, f32 viewportWidth, f32 viewportHeight, f32 zoom) {
        m_perspective = false;

        m_nearPlane = nearPlane;
        m_farPlane = farPlane;

        m_viewportWidth = viewportWidth;
        m_viewportHeight = viewportHeight;
        m_zoom = zoom;

        Reset();
    }

    void Camera::Reset() {
        m_position = glms_vec3_zero();
        m_yaw = 0;
        m_pitch = 0;
        m_view = glms_mat4_identity();

        m_updateProjection = true;
    }

    void Camera::SetViewportSize(f32 width, f32 height) {
        m_viewportWidth = width;
        m_viewportHeight = height;

        m_updateProjection = true;
    }

    void Camera::SetZoom(f32 zoom) {
        m_zoom = zoom;

        m_updateProjection = true;
    }

    void Camera::SetAspectRatio(f32 aspectRatio) {
        m_aspectRatio = aspectRatio;

        m_updateProjection = true;
    }

    void Camera::SetFovY(f32 fovY) {
        m_fovY = fovY;

        m_updateProjection = true;
    }

    void Camera::Update() {
        // Quaternion based rotation.
        // https://stackoverflow.com/questions/49609654/quaternion-based-first-person-view-camera
        const versors pitchRotation = glms_quat(m_pitch, 1, 0, 0);
        const versors yawRotation = glms_quat(m_yaw, 0, 1, 0);
        const versors rotation = glms_quat_normalize(glms_quat_mul(pitchRotation, yawRotation));

        const mat4s translation = glms_translate_make(glms_vec3_scale(m_position, -1.f));
        m_view = glms_mat4_mul(glms_quat_mat4(rotation), translation);

        // Update the vectors used for movement
        m_right = {m_view.m00, m_view.m10, m_view.m20};
        m_up = {m_view.m01, m_view.m11, m_view.m21};
        m_direction = {m_view.m02, m_view.m12, m_view.m22};

        if (m_updateProjection) {
            m_updateProjection = false;

            CalculateProjectionMatrix();
        }

        // Calculate final view projection matrix
        CalculateViewProjection();
    }

    void Camera::Rotate(f32 deltaPitch, f32 deltaYaw) {
        m_pitch += deltaPitch;
        m_yaw += deltaYaw;
    }

    void Camera::CalculateProjectionMatrix() {
        if (m_perspective) {
            m_projection = glms_perspective(glm_rad(m_fovY), m_aspectRatio, m_nearPlane, m_farPlane);
        } else {
            m_projection = glms_ortho(m_zoom * -m_viewportWidth / 2.f, m_zoom * m_viewportWidth / 2.f, m_zoom * -m_viewportHeight / 2.f, m_zoom * m_viewportHeight / 2.f, m_nearPlane, m_farPlane);
        }
    }

    void Camera::CalculateViewProjection() {
        m_viewProjection = glms_mat4_mul(m_projection, m_view);
    }

    vec3s Camera::Unproject(const vec3s &screenCoordinates) {
        return glms_unproject(screenCoordinates, m_viewProjection, {0, 0, m_viewportWidth, m_viewportHeight});
    }

    vec3s Camera::UnprojectInvertedY(const vec3s &screenCoordinates) {
        const vec3s screenCoordinatesYInv{screenCoordinates.x, m_viewportHeight - screenCoordinates.y, screenCoordinates.z};
        return Unproject(screenCoordinatesYInv);
    }

    void Camera::GetProjectionOrtho2D(mat4 &outMatrix) {
        glm_ortho(0, m_viewportWidth * m_zoom, 0, m_viewportHeight * m_zoom, -1.f, 1.f, outMatrix);
    }

    void Camera::YawPitchFromDirection(const vec3s &direction, f32 &yaw, f32 &pitch) {
        yaw = glm_deg(atan2f(direction.z, direction.x));
        pitch = glm_deg(asinf(direction.y));
    }
}