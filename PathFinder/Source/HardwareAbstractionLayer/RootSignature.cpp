#include "RootSignature.hpp"
#include "Utils.h"

namespace HAL
{

    RootSignature::RootSignature()
    {
		mDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    }

    void RootSignature::AddDescriptorTableParameter(const RootDescriptorTableParameter& table)
    {
        mDescriptorTableParameters.push_back(table);
        mD3DParameters.push_back(table.D3DParameter());
        mDesc.NumParameters = mD3DParameters.size();
        mDesc.pParameters = &mD3DParameters[0];
    }

    void RootSignature::AddDescriptorParameter(const RootDescriptorParameter& descriptor)
    {
        mDescriptorParameters.push_back(descriptor);
        mD3DParameters.push_back(descriptor.D3DParameter());
        mDesc.NumParameters = mD3DParameters.size();
        mDesc.pParameters = &mD3DParameters[0];
    }

    void RootSignature::AddConstantsParameter(const RootConstantsParameter& constants)
    {
        mConstantParameters.push_back(constants);
        mD3DParameters.push_back(constants.D3DParameter());
        mDesc.NumParameters = mD3DParameters.size();
        mDesc.pParameters = &mD3DParameters[0];
    }

    void RootSignature::Compile()
    {

    }

}

