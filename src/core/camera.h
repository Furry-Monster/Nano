#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "core/math.h"

namespace Nano
{
    struct Frustum
    {
        // Order:left,right,bottom,top,near,far
        // vec4(A,B,C,D) ---> Ax + By + Cz + D = 0
        // vec3(A,B,C) ---> normal , float D ---> distance to origin(0,0,0).
        glm::vec4 planes[6];

        constexpr bool containsPoint(const glm::vec3& point) const
        {
            for (const glm::vec4& plane : planes)
                if (glm::dot(glm::vec3(plane), point) + plane.w < 0.0f)
                    return false;
            return true;
        }

        constexpr bool containsSphere(const glm::vec3& center, float r) const
        {
            for (const glm::vec4& plane : planes)
                if (glm::dot(glm::vec3(plane), center) + plane.w < -r)
                    return false;
            return true;
        }

        constexpr bool containsAABB(const AABB& bounds) const
        {
            for (const glm::vec4& plane : planes)
            {
                glm::vec3 positive_vert = bounds.min;
                if (plane.x >= 0)
                    positive_vert.x = bounds.max.x;
                if (plane.y >= 0)
                    positive_vert.y = bounds.max.y;
                if (plane.z >= 0)
                    positive_vert.z = bounds.max.z;

                if (glm::dot(glm::vec3(plane), positive_vert) + plane.w < 0.0f)
                    return false;
            }
            return true;
        }
    };

    enum class ProjectionType : uint8_t
    {
        Perspective,
        Orthographic
    };

    class Camera
    {
    public:
        Camera()           = default;
        ~Camera() noexcept = default;

        Camera(const Camera&)                = delete;
        Camera& operator=(const Camera&)     = delete;
        Camera(Camera&&) noexcept            = default;
        Camera& operator=(Camera&&) noexcept = default;

        void setPosition(const glm::vec3& pos);
        void setRotation(const glm::quat& rotat);
        void setRotation(const glm::vec3& eular_angle);
        void lookAt(const glm::vec3& target, const glm::vec3& up = glm::vec3(0, 1, 0));

        const glm::vec3& getPosition() const { return m_position; }
        const glm::quat& getRotation() const { return m_rotation; }
        glm::vec3        getLocalForward() const { return m_rotation * glm::vec3(0, 0, -1); }
        glm::vec3        getLocalRight() const { return m_rotation * glm::vec3(1, 0, 0); }
        glm::vec3        getLocalUp() const { return m_rotation * glm::vec3(0, 1, 0); }

        void setPerspective(float fov, float aspect_ratio, float near_plane, float far_plane);
        void setOrthographic(float left, float right, float bottom, float top, float near_plane, float far_plane);

        void  setFov(float fov);
        void  setAspectRatio(float aspect_ratio);
        void  setNearPlane(float near_plane);
        void  setFarPlane(float far_plane);
        float getFov() const { return m_fov; }
        float getAspectRatio() const { return m_aspect_ratio; }
        float getNearPlane() const { return m_near_plane; }
        float getFarPlane() const { return m_far_plane; }

        ProjectionType getProjectionType() const { return m_projection_type; }

        const glm::mat4& getViewMatrix() const;
        const glm::mat4& getProjMatrix() const;
        const glm::mat4& getViewProjMatrix() const;

        const Frustum& getFrustum() const { return m_frustum; }

        void update();

    private:
        void updateViewMatrix();
        void updateProjectionMatrix();
        void extractFrustum();

        glm::vec3 m_position {0.0f, 0.0f, 0.0f};
        glm::quat m_rotation {1.0f, 0.0f, 0.0f, 0.0f};

        ProjectionType m_projection_type {ProjectionType::Perspective};
        float          m_fov {45.0f};
        float          m_aspect_ratio {16.0f / 9.0f};
        float          m_near_plane {0.1f};
        float          m_far_plane {1000.0f};

        float m_ortho_left {-10.0f};
        float m_ortho_right {10.0f};
        float m_ortho_bottom {-10.0f};
        float m_ortho_top {10.0f};

        glm::mat4 m_view_matrix {1.0f};
        glm::mat4 m_proj_matrix {1.0f};
        glm::mat4 m_view_proj_matrix {1.0f};

        Frustum m_frustum;

        bool m_view_mat_dirty {true};
        bool m_proj_mat_dirty {true};
    };

} // namespace Nano
