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

    void RootSignature::Compile(const Device& device)
    {
        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        ThrowIfFailed(D3D12SerializeRootSignature(&mDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errors));
        ThrowIfFailed(device.D3DPtr()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&mSignature)));
    }

}

