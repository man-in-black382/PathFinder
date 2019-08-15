#pragma once

#include <cstdint>

#include "Descriptor.hpp"

namespace HAL
{

    class RootDescriprorTableRange 
    {
    public:
        inline static uint32_t UnboundedRangeSize = UINT_MAX;

    protected:
        RootDescriprorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE rangeType, uint32_t descriptorHeapIndex, uint32_t rangeSize, uint32_t baseRegister, uint32_t registerSpace);
        virtual ~RootDescriprorTableRange() = 0;

    private:
        D3D12_DESCRIPTOR_RANGE mRange;

    public:
        inline const D3D12_DESCRIPTOR_RANGE& D3DRange() const { return mRange; }
    };

    class CBDescriptorTableRange : public RootDescriprorTableRange 
    {
    public:
        CBDescriptorTableRange(uint32_t baseRegister, uint32_t registerSpace, uint32_t rangeSize = RootDescriprorTableRange::UnboundedRangeSize);
        ~CBDescriptorTableRange() = default;
    };

    class SRDescriptorTableRange : public RootDescriprorTableRange 
    {
    public:
        SRDescriptorTableRange(uint32_t baseRegister, uint32_t registerSpace, uint32_t rangeSize = RootDescriprorTableRange::UnboundedRangeSize);
        ~SRDescriptorTableRange() = default;
    };

    class UADescriptorTableRange : public RootDescriprorTableRange
    {
    public:
        UADescriptorTableRange(uint32_t baseRegister, uint32_t registerSpace, uint32_t rangeSize = RootDescriprorTableRange::UnboundedRangeSize);
        ~UADescriptorTableRange() = default;
    };

    class SamplerDescriptorTableRange : public RootDescriprorTableRange
    {
    public:
        ~SamplerDescriptorTableRange() = default;
    };

}

