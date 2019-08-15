#include "RootParameters.hpp"
#include "Utils.h"

#include "../Foundation/Assert.hpp"

namespace HAL
{

    RootParameter::RootParameter(D3D12_ROOT_PARAMETER_TYPE type)
    {
        mParameter.ParameterType = type;
        mParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }

    RootParameter::~RootParameter() {}



    RootDescriptorTableParameter::RootDescriptorTableParameter() 
        : RootParameter(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {}

    void RootDescriptorTableParameter::AddDescriptorRange(const RootDescriprorTableRange& range)
    {
        assert_format(mRanges.empty() || mRanges.back().NumDescriptors != RootDescriprorTableRange::UnboundedRangeSize,
            "Cannot insert any ranges in a table after an unbounded range");

        mRanges.push_back(range.D3DRange());
        mParameter.DescriptorTable.pDescriptorRanges = &mRanges[0];
        mParameter.DescriptorTable.NumDescriptorRanges = (UINT)mRanges.size();
    }



    RootDescriptorParameter::RootDescriptorParameter(uint32_t shaderRegister, uint32_t registerSpace)
        : RootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV)
    {
        mParameter.Descriptor = { shaderRegister, registerSpace };
    }

}

