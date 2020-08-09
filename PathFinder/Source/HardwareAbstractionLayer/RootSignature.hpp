#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <cstdint>
#include <vector>
#include <unordered_map>

#include "GraphicAPIObject.hpp"
#include "Device.hpp"
#include "RootParameters.hpp"
#include "Sampler.hpp"

namespace HAL
{

    class RootSignature : public GraphicAPIObject
    {
    public:
        struct ParameterIndex
        {
            uint32_t IndexInSignature = 0;
            bool IsIndirect = false; // Descriptor table parameter
        };

        RootSignature(const Device* device);

        void AddDescriptorTableParameter(const RootDescriptorTableParameter& table);
        void AddDescriptorParameter(const RootDescriptorParameter& descriptor);
        void AddConstantsParameter(const RootConstantsParameter& constants);
        void AddStaticSampler(const StaticSampler& sampler);

        RootSignature Clone() const;
        void Compile();
        uint16_t ParameterCount() const;

        std::optional<ParameterIndex> GetParameterIndex(const RootParameter::LocationInSignature& location) const;

        virtual void SetDebugName(const std::string& name) override;

    private:
        void ValidateThenAddParameterLocations(const RootParameter& parameter, bool indirectParameter);

        std::vector<RootDescriptorTableParameter> mDescriptorTableParameters;
        std::vector<RootDescriptorParameter> mDescriptorParameters;
        std::vector<RootConstantsParameter> mConstantParameters;
        std::vector<D3D12_STATIC_SAMPLER_DESC> mD3DStaticSamplers;
        std::vector<D3D12_ROOT_PARAMETER> mD3DParameters;

        std::unordered_map<
            RootParameter::LocationInSignature,
            ParameterIndex,
            RootParameter::LocationHasher,
            RootParameter::LocationEquality>
            mParameterIndices;

        D3D12_ROOT_SIGNATURE_DESC mDesc{};
        Microsoft::WRL::ComPtr<ID3D12RootSignature> mSignature;
        const Device* mDevice;
        std::string mDebugName;

    public:
        inline ID3D12RootSignature* D3DSignature() const { return mSignature.Get(); }
    };

}


