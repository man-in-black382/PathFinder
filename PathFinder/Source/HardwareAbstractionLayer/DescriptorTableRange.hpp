#pragma once

#include <cstdint>

#include "Descriptor.hpp"
#include "ShaderRegister.hpp"

namespace HAL
{

    class RootDescriprorTableRange 
    {
    public:
        inline static uint32_t UnboundedRangeSize = UINT_MAX;

    protected:
        RootDescriprorTableRange(
            D3D12_DESCRIPTOR_RANGE_TYPE rangeType, uint32_t descriptorHeapIndex, uint32_t rangeSize,
            uint32_t baseRegister, uint32_t registerSpace, ShaderRegister registerType);

        virtual ~RootDescriprorTableRange() = 0;

    private:
        D3D12_DESCRIPTOR_RANGE mRange;
        ShaderRegister mRegisterType;

    public:
        inline const D3D12_DESCRIPTOR_RANGE& D3DRange() const { return mRange; }
        inline ShaderRegister ShaderRegisterType() const { return mRegisterType; }
    };

    class CBDescriptorTableRange : public RootDescriprorTableRange 
    {
    public:
        CBDescriptorTableRange(uint32_t baseRegister, uint32_t registerSpace, uint32_t rangeSize = UnboundedRangeSize);
        ~CBDescriptorTableRange() = default;
    };

    class SRDescriptorTableRange : public RootDescriprorTableRange 
    {
    public:
        SRDescriptorTableRange(uint32_t baseRegister, uint32_t registerSpace, uint32_t rangeSize = UnboundedRangeSize);
        ~SRDescriptorTableRange() = default;
    };

    class UADescriptorTableRange : public RootDescriprorTableRange
    {
    public:
        UADescriptorTableRange(uint32_t baseRegister, uint32_t registerSpace, uint32_t rangeSize = UnboundedRangeSize);
        ~UADescriptorTableRange() = default;
    };

    class SamplerDescriptorTableRange : public RootDescriprorTableRange
    {
    public:
        SamplerDescriptorTableRange(uint32_t baseRegister, uint32_t registerSpace, uint32_t rangeSize = UnboundedRangeSize);
        ~SamplerDescriptorTableRange() = default;
    };

}

