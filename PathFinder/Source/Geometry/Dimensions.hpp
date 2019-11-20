#pragma once

#include <cstdint>

namespace Geometry {

    struct Dimensions {
    public:
        Dimensions(uint64_t w, uint64_t h, uint64_t d);
        Dimensions(uint64_t w, uint64_t h);
        Dimensions(uint64_t w);
        Dimensions() = default;

        uint64_t LargestDimension() const;

        bool operator==(const Dimensions &rhs);
        bool operator!=(const Dimensions &rhs);

        uint64_t Width = 1;
        uint64_t Height = 1;
        uint64_t Depth = 1;
    };

}
