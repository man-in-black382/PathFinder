#include "RootParameters.hpp"
#include "Utils.h"

namespace HAL
{

    RootDescriprorTableRange::RootDescriprorTableRange(
        D3D12_DESCRIPTOR_RANGE_TYPE rangeType, uint32_t descriptorHeapIndex, uint32_t rangeSize, 
        uint32_t baseRegister, uint32_t registerSpace, ShaderRegister registerType)
        : mRegisterType{ registerType }
    {
        mRange.RangeType = rangeType;
        mRange.NumDescriptors = rangeSize;
        mRange.OffsetInDescriptorsFromTableStart = descriptorHeapIndex;
        mRange.BaseShaderRegister = baseRegister;
        mRange.RegisterSpace = registerSpace;
    }

    RootDescriprorTableRange::~RootDescriprorTableRange() {}


    CBDescriptorTableRange::CBDescriptorTableRange(uint32_t baseRegister, uint32_t registerSpace, uint32_t rangeSize)
        : RootDescriprorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND, rangeSize, baseRegister, registerSpace, ShaderRegister::ConstantBuffer) {}

    SRDescriptorTableRange::SRDescriptorTableRange(uint32_t baseRegister, uint32_t registerSpace, uint32_t rangeSize)
        : RootDescriprorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND, rangeSize, baseRegister, registerSpace, ShaderRegister::ShaderResource) {}

    UADescriptorTableRange::UADescriptorTableRange(uint32_t baseRegister, uint32_t registerSpace, uint32_t rangeSize)
        : RootDescriprorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND, rangeSize, baseRegister, registerSpace, ShaderRegister::UnorderedAccess) {}

    SamplerDescriptorTableRange::SamplerDescriptorTableRange(uint32_t baseRegister, uint32_t registerSpace, uint32_t rangeSize)
        : RootDescriprorTableRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND, rangeSize, baseRegister, registerSpace, ShaderRegister::Sampler) {}

}

