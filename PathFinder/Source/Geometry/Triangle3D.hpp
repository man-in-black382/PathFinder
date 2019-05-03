#pragma once

#include "Triangle.hpp"

#include <glm/vec3.hpp>

namespace Geometry {

    struct AxisAlighnedBox3D;

    struct Triangle3D : public Triangle<glm::vec3> {
        using Triangle<glm::vec3>::Triangle;

        float area() const override;

        AxisAlignedBox3D boundingBox() const;

        glm::vec3 normal() const;

        std::array<Triangle3D, 4> split() const;
    };

}
