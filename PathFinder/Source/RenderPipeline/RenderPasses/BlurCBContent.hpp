#pragma once

#include <cstdint>
#include <array>
#include <glm/vec2.hpp>

namespace PathFinder
{

    struct BlurCBContent
    {
        static const int MaximumRadius = 64;

        std::array<float, MaximumRadius> Weights;
        glm::vec2 ImageSize;
        uint32_t IsHorizontal;
        uint32_t BlurRadius;
        uint32_t InputTexIdx;
        uint32_t OutputTexIdx;
    };

}
