#include "StaticSampler.hpp"

namespace HAL
{

    const HAL::StaticSampler StaticSampler::AnisotropicClamp(uint16_t shaderResiter, uint16_t registerSpace)
    {
        StaticSampler s;
        s.mD3DStaticSampler.Filter = D3D12_FILTER_ANISOTROPIC;
        s.mD3DStaticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        s.mD3DStaticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        s.mD3DStaticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        s.mD3DStaticSampler.MipLODBias = 0;
        s.mD3DStaticSampler.MaxAnisotropy = 16;
        s.mD3DStaticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        s.mD3DStaticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        s.mD3DStaticSampler.MinLOD = 0;
        s.mD3DStaticSampler.MaxLOD = D3D12_FLOAT32_MAX;
        s.mD3DStaticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        s.mD3DStaticSampler.ShaderRegister = shaderResiter;
        s.mD3DStaticSampler.RegisterSpace = registerSpace;

        return s;
    }

    const HAL::StaticSampler StaticSampler::LinearClamp(uint16_t shaderResiter, uint16_t registerSpace)
    {
        StaticSampler s;
        s.mD3DStaticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        s.mD3DStaticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        s.mD3DStaticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        s.mD3DStaticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        s.mD3DStaticSampler.MipLODBias = 0;
        s.mD3DStaticSampler.MaxAnisotropy = 16;
        s.mD3DStaticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        s.mD3DStaticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        s.mD3DStaticSampler.MinLOD = 0;
        s.mD3DStaticSampler.MaxLOD = D3D12_FLOAT32_MAX;
        s.mD3DStaticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        s.mD3DStaticSampler.ShaderRegister = shaderResiter;
        s.mD3DStaticSampler.RegisterSpace = registerSpace;

        return s;
    }

    const HAL::StaticSampler StaticSampler::PointClamp(uint16_t shaderResiter, uint16_t registerSpace)
    {
        StaticSampler s;
        s.mD3DStaticSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        s.mD3DStaticSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        s.mD3DStaticSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        s.mD3DStaticSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        s.mD3DStaticSampler.MipLODBias = 0;
        s.mD3DStaticSampler.MaxAnisotropy = 16;
        s.mD3DStaticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        s.mD3DStaticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        s.mD3DStaticSampler.MinLOD = 0;
        s.mD3DStaticSampler.MaxLOD = D3D12_FLOAT32_MAX;
        s.mD3DStaticSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        s.mD3DStaticSampler.ShaderRegister = shaderResiter;
        s.mD3DStaticSampler.RegisterSpace = registerSpace;

        return s;
    }

}
