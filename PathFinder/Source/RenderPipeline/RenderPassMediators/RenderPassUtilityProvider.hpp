#pragma once

#include <cstdint>

#include "../RenderSurfaceDescription.hpp"

namespace PathFinder
{

    class RenderPassUtilityProvider
    {
    public:
        uint64_t FrameNumber;
        RenderSurfaceDescription DefaultRenderSurfaceDescription;
    };

}
