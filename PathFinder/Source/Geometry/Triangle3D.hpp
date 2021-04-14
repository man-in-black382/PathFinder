#pragma once

#include "Triangle.hpp"

#include <glm/vec3.hpp>

namespace Geometry
{

    struct AxisAlighnedBox3D;

    struct Triangle3D : public Triangle<glm::vec3>
    {
        using Triangle<glm::vec3>::Triangle;

        float GetArea() const override;
        glm::vec3 GetNormal() const;
    };

}
