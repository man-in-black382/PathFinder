#pragma once

#include <cstdint>

#include "Descriptor.hpp"

namespace HAL
{

    class RootDescriprorTableRange {
    protected:
        RootDescriprorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE rangeType, uint32_t descriptorHeapIndex, uint32_t rangeSize, uint32_t baseRegister, uint32_t registerSpace);
        virtual ~RootDescriprorTableRange() = 0;

    private:
        D3D12_DESCRIPTOR_RANGE mRange;

    public:
        inline const auto& D3DRange() const { return mRange; }
    };

    class CBSRUADescriptorTableRange : public RootDescriprorTableRange {
    public:
        CBSRUADescriptorTableRange(const CBDescriptor& rangeStartDescriptor, uint32_t rangeSize, uint32_t baseRegister, uint32_t registerSpace = 0);
        CBSRUADescriptorTableRange(const SRDescriptor& rangeStartDescriptor, uint32_t rangeSize, uint32_t baseRegister, uint32_t registerSpace = 0);
        CBSRUADescriptorTableRange(const UADescriptor& rangeStartDescriptor, uint32_t rangeSize, uint32_t baseRegister, uint32_t registerSpace = 0);

        ~CBSRUADescriptorTableRange() = default;
    };

    class SamplerDescriptorTableRange : public RootDescriprorTableRange {
    public:
        ~SamplerDescriptorTableRange() = default;
    };

}

