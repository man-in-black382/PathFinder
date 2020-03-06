#pragma once

#include <d3d12.h>
#include <cstdint>

namespace HAL
{

    class RayTracingPipelineConfig
    {
    public:
        RayTracingPipelineConfig(uint32_t maxTracingRecursionDepth);

    private:
        D3D12_RAYTRACING_PIPELINE_CONFIG mConfig{};
        uint32_t mMaxTracingRecursionDepth = 1;

    public:
        inline const D3D12_RAYTRACING_PIPELINE_CONFIG& D3DConfig() const { return mConfig; }
        inline auto MaxTracingRecursionDepth() const { return mMaxTracingRecursionDepth; }
    };

}

