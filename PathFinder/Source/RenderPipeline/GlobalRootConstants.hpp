#pragma once

#include <glm/vec2.hpp>

namespace PathFinder
{

    struct GlobalRootConstants
    {
        glm::vec2 PipelineRTResolution;
        glm::vec2 PipelineRTResolutionInverse;
        uint32_t AnisotropicClampSamplerIdx;
        uint32_t LinearClampSamplerIdx;
        uint32_t PointClampSamplerIdx;
        uint32_t MinSamplerIdx;
        uint32_t MaxSamplerIdx;
    };

}
