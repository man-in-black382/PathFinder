#pragma once

#include <Foundation/Name.hpp>
#include <HardwareAbstractionLayer/RootParameters.hpp>

namespace PathFinder
{
    // Only buffers need to be set by user explicitly.
    // Textures are bound to the pipeline automatically by the engine,
    // since they can be freely indexed as unbounded texture arrays.
    // Constant buffers are bound as root constant buffer views.
    // Shader resource and unordered access buffers are bound through 
    // descriptor tables.
    //
    class RootSignatureProxy
    {
    public:
        template <class ConstantsType>
        void AddRootConstantsParameter(uint16_t bRegisterIndex, uint16_t registerSpace);
        void AddConstantBufferParameter(uint16_t bRegisterIndex, uint16_t registerSpace);
        void AddShaderResourceBufferParameter(uint16_t tRegisterIndex, uint16_t registerSpace);
        void AddUnorderedAccessBufferParameter(uint16_t uRegisterIndex, uint16_t registerSpace);

    private:
        std::vector<HAL::RootConstantsParameter> mRootConstantsParameters;
        std::vector<HAL::RootConstantBufferParameter> mRootConstantBufferParameters;
        std::vector<HAL::RootDescriptorTableParameter> mDescriptorTableParameters;

    public:
        inline const auto& RootConstantsParameters() const { return mRootConstantsParameters; }
        inline const auto& RootConstantBufferParameters() const { return mRootConstantBufferParameters; }
        inline const auto& RootDescriptorTableParameters() const { return mDescriptorTableParameters; }
    };

}

#include "RootSignatureProxy.inl"