#pragma once

#include "Transformation.hpp"

namespace Geometry {

    struct Parallelogram3D {
        glm::vec3 corner;
        glm::vec3 side1;
        glm::vec3 side2;

        Parallelogram3D(const glm::vec3 &corner, const glm::vec3 &sideVector1, const glm::vec3 &sideVector2);

        glm::vec3 normal() const;

        Parallelogram3D transformedBy(const Transformation &t) const;

        Parallelogram3D transformedBy(const glm::mat4 &mat) const;
    };

}
