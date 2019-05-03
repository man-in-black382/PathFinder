#pragma once

#include "Triangle.hpp"
#include "Rect2D.hpp"

#include <glm/vec2.hpp>

namespace Geometry {

    struct Triangle2D : public Triangle<glm::vec2> {
        using Triangle<glm::vec2>::Triangle;

        float area() const override;

        Rect2D boundingRect() const;

        std::array<Triangle2D, 4> split() const;
    };

}
