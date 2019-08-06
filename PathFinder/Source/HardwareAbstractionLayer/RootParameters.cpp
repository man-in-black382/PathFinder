#include "RootParameters.hpp"
#include "Utils.h"

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

    void RootDescriptorTableParameter::AddDescriptorRange(const CBSRUADescriptorTableRange& range)
    {
        mRanges.push_back(range.D3DRange());
        mParameter.DescriptorTable.pDescriptorRanges = &mRanges[0];
        mParameter.DescriptorTable.NumDescriptorRanges = (UINT)mRanges.size();
    }
    
    void RootDescriptorTableParameter::AddDescriptorRange(const SamplerDescriptorTableRange& range)
    {
        throw std::runtime_error("Not implemented");
    }



    RootDescriptorParameter::RootDescriptorParameter(uint32_t shaderRegister, uint32_t registerSpace)
        : RootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV)
    {
        mParameter.Descriptor = { shaderRegister, registerSpace };
    }

}

