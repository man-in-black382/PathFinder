#include "RootSignature.hpp"
#include "Utils.h"

namespace HAL
{

    RootSignature::RootSignature()
    {
        mDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }

    void RootSignature::AddDescriptorTableParameter(const RootDescriptorTableParameter& table)
    {
        mDescriptorTableParameters.push_back(table);
        mD3DParameters.push_back(table.D3DParameter());
        mDesc.NumParameters = (UINT)mD3DParameters.size();
        mDesc.pParameters = &mD3DParameters[0];

        //mParameterIndices[GenerateParameterKey(table.D3DParameter().DescriptorTable.)]
    }

    void RootSignature::AddDescriptorParameter(const RootDescriptorParameter& descriptor)
    {
        mDescriptorParameters.push_back(descriptor);
        mD3DParameters.push_back(descriptor.D3DParameter());
        mDesc.NumParameters = (UINT)mD3DParameters.size();
        mDesc.pParameters = &mD3DParameters[0];
    }

    void RootSignature::AddConstantsParameter(const RootConstantsParameter& constants)
    {
        mConstantParameters.push_back(constants);
        mD3DParameters.push_back(constants.D3DParameter());
        mDesc.NumParameters = (UINT)mD3DParameters.size();
        mDesc.pParameters = &mD3DParameters[0];
    }

    void RootSignature::Compile(const Device& device)
    {
        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        ThrowIfFailed(D3D12SerializeRootSignature(&mDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errors));

        if (errors) OutputDebugStringA((char*)errors->GetBufferPointer());

        ThrowIfFailed(device.D3DPtr()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&mSignature)));
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

