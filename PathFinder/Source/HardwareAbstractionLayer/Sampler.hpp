#pragma once

#include <d3d12.h>
#include <cstdint>

namespace HAL
{

    class Sampler
    {
    public:
        enum class MinMaxFilter
        {
            Minimum, Maximum
        };

        enum class UnifiedAlgorithm
        {
            Anisotropic, Linear, Point
        };

        enum class SeparableAlgorithm
        {
            Linear, Point
        };

        enum class Comparator
        {
            Never, Less, Equal, LessOrEqual, Greater, NotEqual, GreaterOrEqual, Always
        };

        enum class AddressMode
        {
            Clamp, Wrap, Mirror
        };

        Sampler(UnifiedAlgorithm algorithm, AddressMode addressMode);
        Sampler(Comparator comparisonFunc, AddressMode addressMode);
        Sampler(MinMaxFilter filter, AddressMode addressMode);

        // TODO: Implement constructor that accepts separate filters and does not cause combinatorial explosion
        /*Sampler(SeparableAlgorithm minification, SeparableAlgorithm magnification, SeparableAlgorithm mipmapping, AddressMode addressMode);*/

    private:
        D3D12_TEXTURE_ADDRESS_MODE GetD3DAddressMode(AddressMode mode) const;
        D3D12_COMPARISON_FUNC GetD3DComparisonFunc(Comparator comparator) const;

        D3D12_SAMPLER_DESC mD3DSampler{};

    public:
        inline const auto& D3DSampler() const { return mD3DSampler; }
    };



    class StaticSampler
    {
    public:
        // TODO: Implement 

    private:
        D3D12_STATIC_SAMPLER_DESC mD3DStaticSampler{};

    public:
        inline const auto& D3DStaticSampler() const { return mD3DStaticSampler; }
    };

}
