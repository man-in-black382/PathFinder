#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <cstdint>
#include <vector>
#include <unordered_map>

#include "GraphicAPIObject.hpp"
#include "Device.hpp"
#include "RootParameters.hpp"

namespace HAL
{

    class RootSignature : public GraphicAPIObject
    {
    public:
        RootSignature(const Device* device);

        void AddDescriptorTableParameter(const RootDescriptorTableParameter& table);
        void AddDescriptorParameter(const RootDescriptorParameter& descriptor);
        void AddConstantsParameter(const RootConstantsParameter& constants);

        RootSignature Clone();
        void Compile();
        uint16_t ParameterCount() const;

        virtual void SetDebugName(const std::string& name) override;

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
        const Device* mDevice;

    public:
        inline ID3D12RootSignature* D3DSignature() const { return mSignature.Get(); }
    };

}

