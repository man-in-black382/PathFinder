#include "RayTracingPipelineConfig.hpp"

namespace HAL
{

    RayTracingPipelineConfig::RayTracingPipelineConfig(uint32_t maxTracingRecursionDepth)
    {
        mConfig.MaxTraceRecursionDepth = maxTracingRecursionDepth;
    }

    RayTracingPipelineConfig::RayTracingPipelineConfig()
        : RayTracingPipelineConfig(1) {}

}
