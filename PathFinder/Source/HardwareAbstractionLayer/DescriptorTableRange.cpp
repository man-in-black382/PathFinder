#include "RootParameters.hpp"
#include "Utils.h"

namespace HAL
{

    RootDescriprorTableRange::RootDescriprorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE rangeType, uint32_t descriptorHeapIndex, uint32_t rangeSize, uint32_t baseRegister, uint32_t registerSpace)
    {
        mRange.RangeType = rangeType;
        mRange.NumDescriptors = rangeSize;
        mRange.OffsetInDescriptorsFromTableStart = descriptorHeapIndex;
        mRange.BaseShaderRegister = baseRegister;
        mRange.RegisterSpace = registerSpace;
    }

    RootDescriprorTableRange::~RootDescriprorTableRange() {}


    CBSRUADescriptorTableRange::CBSRUADescriptorTableRange(const CBDescriptor& rangeStartDescriptor, uint32_t rangeSize, uint32_t baseRegister, uint32_t registerSpace)
        : RootDescriprorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, rangeStartDescriptor.IndexInHeap(), rangeSize, baseRegister, registerSpace) {}

    CBSRUADescriptorTableRange::CBSRUADescriptorTableRange(const SRDescriptor& rangeStartDescriptor, uint32_t rangeSize, uint32_t baseRegister, uint32_t registerSpace)
        : RootDescriprorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, rangeStartDescriptor.IndexInHeap(), rangeSize, baseRegister, registerSpace) {}

    CBSRUADescriptorTableRange::CBSRUADescriptorTableRange(const UADescriptor& rangeStartDescriptor, uint32_t rangeSize, uint32_t baseRegister, uint32_t registerSpace)
        : RootDescriprorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, rangeStartDescriptor.IndexInHeap(), rangeSize, baseRegister, registerSpace) {}

}

