#include "Dimensions.hpp"
#include <algorithm>

namespace Geometry
{

    Dimensions::Dimensions(uint64_t w, uint64_t h, uint64_t d)
        : Width(w), Height(h), Depth(d) {}

    Dimensions::Dimensions(uint64_t w, uint64_t h) 
        : Dimensions(w, h, 1) {}

    Dimensions::Dimensions(uint64_t w)
        : Dimensions(w, 1) {}

    uint64_t Dimensions::LargestDimension() const
    {
        return std::max(std::max(Height, Width), Depth);
    }

    bool Dimensions::operator!=(const Dimensions& rhs)
    {
        return !(*this == rhs); 
    }

    bool Dimensions::operator==(const Dimensions &rhs)
    {
        return Width == rhs.Width && Height == rhs.Height && Depth == rhs.Depth;
    }

    Dimensions Dimensions::XMultiplied(float m) const
    {
        return { uint64_t(Width * m), Height, Depth };
    }

    Dimensions Dimensions::XYMultiplied(float m) const
    {
        return { uint64_t(Width * m), uint64_t(Height * m), Depth };
    }

    Dimensions Dimensions::XYZMultiplied(float m) const
    {
        return { uint64_t(Width * m), uint64_t(Height * m), uint64_t(Depth * m) };
    }

}
