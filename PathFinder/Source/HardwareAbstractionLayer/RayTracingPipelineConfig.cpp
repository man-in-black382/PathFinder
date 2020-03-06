#include "RayTracingPipelineConfig.hpp"

namespace HAL
{

    RayTracingPipelineConfig::RayTracingPipelineConfig(uint32_t maxTracingRecursionDepth)
        : mMaxTracingRecursionDepth{ maxTracingRecursionDepth }
    {
        mConfig.MaxTraceRecursionDepth = maxTracingRecursionDepth;
    }

}
