#pragma once

#include <glm/vec3.hpp>

#include "Sphere.hpp"
#include "AABB.hpp"

namespace Geometry
{

    float Snap(float value, float snap);
    glm::vec3 Snap(const glm::vec3& value, const glm::vec3& snap);

    AABB GetCircumscribedAABBForSphere(const Sphere& sphere);
    Sphere ConvertAABBToSphere(const AABB& aabb);

}
