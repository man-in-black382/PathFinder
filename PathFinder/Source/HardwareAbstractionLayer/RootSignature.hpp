#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <cstdint>
#include <vector>
#include <unordered_map>

#include "Device.hpp"
#include "RootParameters.hpp"

namespace HAL
{

    class RootSignature {
    public:
        RootSignature();

        void AddDescriptorTableParameter(const RootDescriptorTableParameter& table);
        void AddDescriptorParameter(const RootDescriptorParameter& descriptor);
        void AddConstantsParameter(const RootConstantsParameter& constants);

        void Compile(const Device& device);

    private:
        using ParameterKey = uint64_t;

        ParameterKey GenerateParameterKey(uint32_t shaderRegister, uint32_t registerSpace);

        std::vector<RootDescriptorTableParameter> mDescriptorTableParameters;
        std::vector<RootDescriptorParameter> mDescriptorParameters;
        std::vector<RootConstantsParameter> mConstantParameters;
        std::vector<D3D12_ROOT_PARAMETER> mD3DParameters;

        std::unordered_map<ParameterKey, uint32_t> mParameterIndices;

        D3D12_ROOT_SIGNATURE_DESC mDesc{};
        Microsoft::WRL::ComPtr<ID3D12RootSignature> mSignature;

    public:
        inline ID3D12RootSignature* D3DSignature() const { return mSignature.Get(); }
    };

}

