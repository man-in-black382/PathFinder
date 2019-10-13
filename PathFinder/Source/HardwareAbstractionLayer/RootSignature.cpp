#include "RootSignature.hpp"
#include "Utils.h"

#include "../Foundation/StringUtils.hpp"

namespace HAL
{

    RootSignature::RootSignature(const Device* device)
        : mDevice{ device }
    {
        mDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }

    void RootSignature::AddDescriptorTableParameter(const RootDescriptorTableParameter& table)
    {
        mDescriptorTableParameters.push_back(table);
        mD3DParameters.push_back(table.D3DParameter());
    }

    void RootSignature::AddDescriptorParameter(const RootDescriptorParameter& descriptor)
    {
        mDescriptorParameters.push_back(descriptor);
        mD3DParameters.push_back(descriptor.D3DParameter());
    }

    void RootSignature::AddConstantsParameter(const RootConstantsParameter& constants)
    {
        mConstantParameters.push_back(constants);
        mD3DParameters.push_back(constants.D3DParameter());
    }

    RootSignature RootSignature::Clone()
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

        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        ThrowIfFailed(D3D12SerializeRootSignature(&mDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errors));

        if (errors) OutputDebugStringA((char*)errors->GetBufferPointer());

        ThrowIfFailed(mDevice->D3DDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&mSignature)));
    }

    uint16_t RootSignature::ParameterCount() const
    {
        return mDescriptorTableParameters.size() + mDescriptorParameters.size() + mConstantParameters.size();
    }

    void RootSignature::SetDebugName(const std::string& name)
    {
        mSignature->SetName(StringToWString(name).c_str());
    }

    RootSignature::ParameterKey RootSignature::GenerateParameterKey(uint32_t shaderRegister, uint32_t registerSpace)
    {
        uint64_t key = 0;
        key |= shaderRegister;
        key <<= 32;
        key |= registerSpace;
        return key;
    }

}

