#pragma once

#include "Size2D.hpp"
#include <glm/vec2.hpp>

namespace Geometry {

    struct Rect2D
    {
        glm::vec2 Origin;
        Size2D Size;

        static const Rect2D &Zero();

        Rect2D() = default;
        Rect2D(const Size2D &size);
        Rect2D(const glm::vec2 &origin, const Size2D &size);

        float MinX() const;
        float MinY() const;
        float MaxX() const;
        float MaxY() const;

        bool Intersects(const Rect2D& otherRect, float& intersectionArea) const;
    };

}
