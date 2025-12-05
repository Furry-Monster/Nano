#include "core/camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

namespace Nano
{
    void Camera::setPosition(const glm::vec3& pos)
    {
        m_position       = pos;
        m_view_mat_dirty = true;
    }

    void Camera::setRotation(const glm::quat& rotat)
    {
        m_rotation       = glm::normalize(rotat);
        m_view_mat_dirty = true;
    }

    void Camera::setRotation(const glm::vec3& eular_angle)
    {
        glm::quat q_pitch = glm::angleAxis(glm::radians(eular_angle.x), glm::vec3(1, 0, 0));
        glm::quat q_yaw   = glm::angleAxis(glm::radians(eular_angle.y), glm::vec3(0, 1, 0));
        glm::quat q_roll  = glm::angleAxis(glm::radians(eular_angle.z), glm::vec3(0, 0, 1));

        m_rotation       = glm::normalize(q_yaw * q_pitch * q_roll);
        m_view_mat_dirty = true;
    }

    void Camera::lookAt(const glm::vec3& target, const glm::vec3& up)
    {
        glm::mat4 view_matrix = glm::lookAt(m_position, target, up);

        glm::mat3 rotation_part = glm::mat3(view_matrix);
        rotation_part           = glm::transpose(rotation_part);

        m_rotation       = glm::normalize(glm::quat_cast(rotation_part));
        m_view_mat_dirty = true;
    }

    void Camera::setPerspective(float fov, float aspect_ratio, float near_plane, float far_plane)
    {
        m_projection_type = ProjectionType::Perspective;
        m_fov             = fov;
        m_aspect_ratio    = aspect_ratio;
        m_near_plane      = near_plane;
        m_far_plane       = far_plane;
        m_proj_mat_dirty  = true;
    }

    void Camera::setOrthographic(float left, float right, float bottom, float top, float near_plane, float far_plane)
    {
        m_projection_type = ProjectionType::Orthographic;
        m_ortho_left      = left;
        m_ortho_right     = right;
        m_ortho_bottom    = bottom;
        m_ortho_top       = top;
        m_near_plane      = near_plane;
        m_far_plane       = far_plane;
        m_proj_mat_dirty  = true;
    }

    void Camera::setFov(float fov)
    {
        m_fov            = fov;
        m_proj_mat_dirty = true;
    }

    void Camera::setAspectRatio(float aspect_ratio)
    {
        m_aspect_ratio   = aspect_ratio;
        m_proj_mat_dirty = true;
    }

    void Camera::setNearPlane(float near_plane)
    {
        m_near_plane     = near_plane;
        m_proj_mat_dirty = true;
    }

    void Camera::setFarPlane(float far_plane)
    {
        m_far_plane      = far_plane;
        m_proj_mat_dirty = true;
    }

    const glm::mat4& Camera::getViewMatrix() const
    {
        if (m_view_mat_dirty)
        {
            const_cast<Camera*>(this)->updateViewMatrix();
            const_cast<Camera*>(this)->m_view_mat_dirty = false;
        }
        return m_view_matrix;
    }

    const glm::mat4& Camera::getProjMatrix() const
    {
        if (m_proj_mat_dirty)
        {
            const_cast<Camera*>(this)->updateProjectionMatrix();
            const_cast<Camera*>(this)->m_proj_mat_dirty = false;
        }
        return m_proj_matrix;
    }

    const glm::mat4& Camera::getViewProjMatrix() const { return m_view_proj_matrix; }

    void Camera::update()
    {
        if (!m_view_mat_dirty && !m_proj_mat_dirty)
            return;

        if (m_view_mat_dirty)
        {
            updateViewMatrix();
            m_view_mat_dirty = false;
        }

        if (m_proj_mat_dirty)
        {
            updateProjectionMatrix();
            m_proj_mat_dirty = false;
        }

        m_view_proj_matrix = m_proj_matrix * m_view_matrix;
        extractFrustum();
    }

    void Camera::updateViewMatrix()
    {
        glm::mat4 rotation_matrix    = glm::transpose(glm::mat4_cast(m_rotation));
        glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), -m_position);
        m_view_matrix                = rotation_matrix * translation_matrix;
    }

    void Camera::updateProjectionMatrix()
    {
        switch (m_projection_type)
        {
            case ProjectionType::Perspective:
                m_proj_matrix = glm::perspective(glm::radians(m_fov), m_aspect_ratio, m_near_plane, m_far_plane);
                break;
            case ProjectionType::Orthographic:
                m_proj_matrix =
                    glm::ortho(m_ortho_left, m_ortho_right, m_ortho_bottom, m_ortho_top, m_near_plane, m_far_plane);
                break;
        }
    }

    void Camera::extractFrustum()
    {
        glm::mat4 vp = m_view_proj_matrix;

        m_frustum.planes[0] =
            glm::vec4(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0], vp[3][3] + vp[3][0]);
        m_frustum.planes[1] =
            glm::vec4(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0], vp[3][3] - vp[3][0]);
        m_frustum.planes[2] =
            glm::vec4(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1], vp[3][3] + vp[3][1]);
        m_frustum.planes[3] =
            glm::vec4(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1], vp[3][3] - vp[3][1]);
        m_frustum.planes[4] =
            glm::vec4(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2], vp[3][3] + vp[3][2]);
        m_frustum.planes[5] =
            glm::vec4(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2], vp[3][3] - vp[3][2]);

        for (auto& plane : m_frustum.planes)
        {
            float length = glm::length(glm::vec3(plane));
            plane /= length;
        }
    }

} // namespace Nano
