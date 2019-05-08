#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <cstdint>
#include <vector>

#include "RootParameters.hpp"

namespace HAL
{

    class RootSignature {
    public:
        RootSignature();

        void AddDescriptorTableParameter(const RootDescriptorTableParameter& table);
        void AddDescriptorParameter(const RootDescriptorParameter& descriptor);
        void AddConstantsParameter(const RootConstantsParameter& constants);

        void Compile();

    private:
        std::vector<RootDescriptorTableParameter> mDescriptorTableParameters;
        std::vector<RootDescriptorParameter> mDescriptorParameters;
        std::vector<RootConstantsParameter> mConstantParameters;
        std::vector<D3D12_ROOT_PARAMETER> mD3DParameters;

        D3D12_ROOT_SIGNATURE_DESC mDesc;
    };

}

