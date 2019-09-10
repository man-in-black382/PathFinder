#pragma once

#include <d3d12.h>
#include <cstdint>

namespace HAL
{

    class RayTracingShaderConfig
    {
    public:
        RayTracingShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributesSize);

    private:
        D3D12_RAYTRACING_SHADER_CONFIG mConfig{};

    public:
        inline const D3D12_RAYTRACING_SHADER_CONFIG& D3DConfig() const { return mConfig; }
    };

}

