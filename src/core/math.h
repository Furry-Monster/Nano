#pragma once

#include <glm/glm.hpp>

namespace Nano
{
    struct AABB
    {
        glm::vec3 min;
        glm::vec3 max;

        constexpr glm::vec3 center() const { return (min + max) * 0.5f; }
        constexpr glm::vec3 extent() const { return max - min; }
    };

} // namespace Nano

