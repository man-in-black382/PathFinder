#pragma once

#include <glm/vec3.hpp>

namespace Geometry
{
    float Snap(float value, float snap);
    glm::vec3 Snap(const glm::vec3& value, const glm::vec3& snap);
}
