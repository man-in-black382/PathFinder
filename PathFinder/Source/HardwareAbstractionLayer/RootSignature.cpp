#include "RootSignature.hpp"
#include "Utils.h"

#include <Foundation/StringUtils.hpp>


namespace HAL
{

    RootSignature::RootSignature(const Device* device)
        : mDevice{ device } {}

    void RootSignature::AddDescriptorTableParameter(const RootDescriptorTableParameter& table)
    {
        ValidateThenAddParameterLocations(table, true);
        mDescriptorTableParameters.push_back(table);
        mD3DParameters.push_back(table.D3DParameter());
    }

    void RootSignature::AddDescriptorParameter(const RootDescriptorParameter& descriptor)
    {
        ValidateThenAddParameterLocations(descriptor, false);
        mDescriptorParameters.push_back(descriptor);
        mD3DParameters.push_back(descriptor.D3DParameter());
    }

    void RootSignature::AddConstantsParameter(const RootConstantsParameter& constants)
    {
        ValidateThenAddParameterLocations(constants, false);
        mConstantParameters.push_back(constants);
        mD3DParameters.push_back(constants.D3DParameter());
    }

    void RootSignature::AddStaticSampler(const StaticSampler& sampler)
    {
        mD3DStaticSamplers.push_back(sampler.D3DStaticSampler());
    }

    RootSignature RootSignature::Clone() const 
    {
        RootSignature newSignature = *this;
        newSignature.mSignature = nullptr;
        return newSignature;
    }

    void RootSignature::Compile()
    {
        // Reassign pointers that might have been lost after RootSignature moves/copies
        //
        uint32_t tableParameterIndex = 0;

        for (D3D12_ROOT_PARAMETER& d3dParameter : mD3DParameters)
        {
            if (d3dParameter.ParameterType != D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) continue;

            RootDescriptorTableParameter& table = mDescriptorTableParameters[tableParameterIndex];

            d3dParameter.DescriptorTable.NumDescriptorRanges = table.D3DParameter().DescriptorTable.NumDescriptorRanges;
            d3dParameter.DescriptorTable.pDescriptorRanges = table.D3DParameter().DescriptorTable.pDescriptorRanges;

            tableParameterIndex++;
        }

        mDesc.NumParameters = (UINT)mD3DParameters.size();
        mDesc.pParameters = &mD3DParameters[0];

        mDesc.NumStaticSamplers = (UINT)mD3DStaticSamplers.size();
        mDesc.pStaticSamplers = mD3DStaticSamplers.data();

        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        ThrowIfFailed(D3D12SerializeRootSignature(&mDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errors));

        if (errors) OutputDebugStringA((char*)errors->GetBufferPointer());

        ThrowIfFailed(mDevice->D3DDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&mSignature)));
    
        mSignature->SetName(StringToWString(mDebugName).c_str());
    }

    uint16_t RootSignature::ParameterCount() const
    {
        return mDescriptorTableParameters.size() + mDescriptorParameters.size() + mConstantParameters.size();
    }

    std::optional<RootSignature::ParameterIndex> RootSignature::GetParameterIndex(const RootParameter::LocationInSignature& location) const
    {
        auto it = mParameterIndices.find(location);
        return it != mParameterIndices.end() ? std::optional(it->second) : std::nullopt;
    }

    void RootSignature::SetDebugName(const std::string& name)
    {
        if (mSignature)
        {
            mSignature->SetName(StringToWString(name).c_str());
        }

        mDebugName = name;
    }

    void RootSignature::ValidateThenAddParameterLocations(const RootParameter& parameter, bool indirectParameter)
    {
        for (const RootParameter::LocationInSignature& location : parameter.SignatureLocations())
        {
            bool registerSpaceOccupied = mParameterIndices.find(location) != mParameterIndices.end();
            assert_format(!registerSpaceOccupied, "Register of such slot, space and type is already occupied in this root signature");
            mParameterIndices[location] = { (uint32_t)mD3DParameters.size(), indirectParameter };
        }
    }

}

