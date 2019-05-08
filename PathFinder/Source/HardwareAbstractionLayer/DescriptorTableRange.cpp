#include "RootParameters.hpp"
#include "Utils.h"

namespace HAL
{

    RootDescriprorTableRange::RootDescriprorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE rangeType, uint32_t descriptorHeapIndex, uint32_t rangeSize, uint32_t registerSlot, uint32_t registerSpace)
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

    
    
    RootParameter::RootParameter(D3D12_ROOT_PARAMETER_TYPE type)
    {
        mParameter.ParameterType = type;
    }

    RootParameter::~RootParameter() {}

    RootDescriptorTableParameter::RootDescriptorTableParameter() : RootParameter(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {}

    void RootDescriptorTableParameter::AddDescriptorRange(const CBSRUADescriptorTableRange& range)
    {
        mRanges.push_back(range.D3DRange());
        mParameter.DescriptorTable.pDescriptorRanges = &mRanges[0];
        mParameter.DescriptorTable.NumDescriptorRanges = mRanges.size();
    }

    void RootDescriptorTableParameter::AddDescriptorRange(const SamplerDescriptorTableRange& range)
    {
        throw std::runtime_error("Not implemented");
    }

}

