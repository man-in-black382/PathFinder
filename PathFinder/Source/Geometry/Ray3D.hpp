#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "AxisAlignedBox3D.hpp"
#include "Parallelogram3D.hpp"
#include "Transformation.hpp"

namespace Geometry {

    struct Ray3D {
        glm::vec3 origin;
        glm::vec3 direction;

        Ray3D(const glm::vec3 &origin, const glm::vec3 &direction);

        Ray3D transformedBy(const Transformation &t) const;

        Ray3D transformedBy(const glm::mat4 &m) const;
    };

}
