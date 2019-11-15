#include "Dimensions.hpp"
#include <algorithm>

namespace Geometry {

    Dimensions::Dimensions(uint64_t w, uint64_t h, uint64_t d) : Width(w), Height(h), Depth(d) {}

    Dimensions::Dimensions(uint64_t w, uint64_t h) : Dimensions(w, h, 1) {}

    Dimensions::Dimensions(uint64_t w) : Dimensions(w, 1) {}

    bool Dimensions::operator!=(const Dimensions &rhs)
    {
        return !(*this == rhs); 
    }

    bool Dimensions::operator==(const Dimensions &rhs)
    {
        return Width == rhs.Width && Height == rhs.Height && Depth == rhs.Depth;
    }

}
