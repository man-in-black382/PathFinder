#include "Size2D.hpp"

#include <algorithm>

namespace Geometry {

    const Size2D &Size2D::Zero()
    {
        static Size2D zero = { 0, 0 };
        return zero;
    }

    const Size2D &Size2D::Unit() 
    {
        static Size2D unit = { 1, 1 };
        return unit;
    }

    Size2D::Size2D(float w, float h)
        : Width(w), Height(h) {}

    Size2D::Size2D(float side)
        : Width(side), Height(side) {}

    bool Size2D::operator==(const Size2D &rhs)
    {
        return Width == rhs.Width && Height == rhs.Height;
    }

    bool Size2D::operator!=(const Size2D &rhs)
    {
        return !operator==(rhs);
    }

    Size2D Size2D::TransformedBy(const glm::vec2 &vector) const 
    {
        return Size2D(Width * vector.x, Height * vector.y);
    }

    Size2D Size2D::MakeUnion(const Size2D &size)
    {
        return { std::max(Width, size.Width), std::max(Height, size.Height) };
    }

}
