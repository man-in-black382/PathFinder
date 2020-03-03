#include "RootSignatureProxy.hpp"

namespace PathFinder
{

    void RootSignatureProxy::AddConstantBufferParameter(uint16_t bRegisterIndex, uint16_t registerSpace)
    {
        mRootConstantBufferParameters.emplace_back(HAL::RootConstantBufferParameter{ bRegisterIndex, registerSpace });
    }

    void RootSignatureProxy::AddShaderResourceBufferParameter(uint16_t tRegisterIndex, uint16_t registerSpace)
    {
        HAL::RootDescriptorTableParameter& table = mDescriptorTableParameters.emplace_back();
        // Shader resource range of size 1
        table.AddDescriptorRange(HAL::SRDescriptorTableRange{ tRegisterIndex, registerSpace, 1 });
    }

    void RootSignatureProxy::AddUnorderedAccessBufferParameter(uint16_t uRegisterIndex, uint16_t registerSpace)
    {
        HAL::RootDescriptorTableParameter& table = mDescriptorTableParameters.emplace_back();
        // Unordered access range of size 1
        table.AddDescriptorRange(HAL::UADescriptorTableRange{ uRegisterIndex, registerSpace, 1 });
    }

}
