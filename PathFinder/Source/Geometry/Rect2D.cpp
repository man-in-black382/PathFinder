#include "Rect2D.hpp"

#include <algorithm>

namespace Geometry
{

    const Rect2D &Rect2D::Zero() 
    {
        static Rect2D zero;
        return zero;
    }

    Rect2D::Rect2D(const Size2D &size)
        :Origin({ 0, 0 }), Size(size) {}

    Rect2D::Rect2D(const glm::vec2 &origin, const Size2D &size)
        : Origin(origin), Size(size) {}

    float Rect2D::MinX() const
    {
        return Origin.x;
    }

    float Rect2D::MinY() const 
    {
        return Origin.y;
    }

    float Rect2D::MaxX() const 
    {
        return Origin.x + Size.Width;
    }

    float Rect2D::MaxY() const 
    {
        return Origin.y + Size.Height;
    }

    bool Rect2D::Intersects(const Rect2D& otherRect, float& intersectionArea) const
    {
        intersectionArea =
            std::max(0.f, std::min(MaxX(), otherRect.MaxX()) - std::max(MinX(), otherRect.MinX())) *
            std::max(0.f, std::min(MaxY(), otherRect.MaxY()) - std::max(MinY(), otherRect.MinY()));

        return intersectionArea > 0.f;
    }

}
