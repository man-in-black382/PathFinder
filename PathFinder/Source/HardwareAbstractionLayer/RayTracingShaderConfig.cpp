#include "RayTracingShaderConfig.hpp"



#include <d3d12.h>

namespace HAL
{

    RayTracingShaderConfig::RayTracingShaderConfig(uint32_t maxPayloadSize, uint32_t maxAttributesSize)
        : mMaxPayloadSize{ maxPayloadSize }, mMaxAttributesSize{ maxAttributesSize }
    {
        assert_format(maxAttributesSize <= D3D12_RAYTRACING_MAX_ATTRIBUTE_SIZE_IN_BYTES, "Exceeded maximum attribute size");
        mConfig.MaxPayloadSizeInBytes = maxPayloadSize;
        mConfig.MaxAttributeSizeInBytes = maxAttributesSize;
    }

}
