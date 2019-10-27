#pragma once

#include <d3d12.h>
#include <cstdint>

namespace HAL
{

    class StaticSampler
    {
    public:
        static const StaticSampler AnisotropicClamp(uint16_t shaderResiter, uint16_t registerSpace = 0);
        static const StaticSampler LinearClamp(uint16_t shaderResiter, uint16_t registerSpace = 0);
        static const StaticSampler PointClamp(uint16_t shaderResiter, uint16_t registerSpace = 0);

    private:
        D3D12_STATIC_SAMPLER_DESC mD3DStaticSampler{};

    public:
        inline const auto& D3DStaticSampler() const { return mD3DStaticSampler; }
    };

}
