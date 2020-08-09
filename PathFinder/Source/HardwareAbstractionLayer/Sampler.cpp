#include "Sampler.hpp"

namespace HAL
{

    Sampler::Sampler(UnifiedAlgorithm algorithm, AddressMode addressMode)
    {
        D3D12_FILTER filter{};
        UINT anisotropy = 0;

        switch (algorithm)
        {
        case HAL::Sampler::UnifiedAlgorithm::Anisotropic: filter = D3D12_FILTER_ANISOTROPIC; anisotropy = 16; break;
        case HAL::Sampler::UnifiedAlgorithm::Linear: filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; break;
        case HAL::Sampler::UnifiedAlgorithm::Point: filter = D3D12_FILTER_MIN_MAG_MIP_POINT; break;
        default: filter = D3D12_FILTER_ANISOTROPIC; break;
        }

        D3D12_TEXTURE_ADDRESS_MODE d3dAddressMode = GetD3DAddressMode(addressMode);

        mD3DSampler.Filter = filter;
        mD3DSampler.AddressU = d3dAddressMode;
        mD3DSampler.AddressV = d3dAddressMode;
        mD3DSampler.AddressW = d3dAddressMode;
        mD3DSampler.MipLODBias = 0;
        mD3DSampler.MaxAnisotropy = anisotropy;
        mD3DSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        mD3DSampler.BorderColor[0] = 0; 
        mD3DSampler.BorderColor[1] = 0;
        mD3DSampler.BorderColor[2] = 0;
        mD3DSampler.BorderColor[3] = 0;
        mD3DSampler.MinLOD = 0;
        mD3DSampler.MaxLOD = D3D12_FLOAT32_MAX;
    }

    Sampler::Sampler(Comparator comparisonFunc, AddressMode addressMode)
    {
        D3D12_TEXTURE_ADDRESS_MODE d3dAddressMode = GetD3DAddressMode(addressMode);
        D3D12_COMPARISON_FUNC d3dComparator = GetD3DComparisonFunc(comparisonFunc);

        mD3DSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
        mD3DSampler.AddressU = d3dAddressMode;
        mD3DSampler.AddressV = d3dAddressMode;
        mD3DSampler.AddressW = d3dAddressMode;
        mD3DSampler.MipLODBias = 0;
        mD3DSampler.MaxAnisotropy = 0;
        mD3DSampler.ComparisonFunc = d3dComparator;
        mD3DSampler.BorderColor[0] = 0;
        mD3DSampler.BorderColor[1] = 0;
        mD3DSampler.BorderColor[2] = 0;
        mD3DSampler.BorderColor[3] = 0;
        mD3DSampler.MinLOD = 0;
        mD3DSampler.MaxLOD = D3D12_FLOAT32_MAX;
    }

    Sampler::Sampler(MinMaxFilter filter, AddressMode addressMode)
    {
        D3D12_FILTER d3dFilter{};

        switch (filter)
        {
        case MinMaxFilter::Minimum: d3dFilter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR; break;
        case MinMaxFilter::Maximum: d3dFilter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR; break;
        default: d3dFilter = D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR; break;
        }

        D3D12_TEXTURE_ADDRESS_MODE d3dAddressMode = GetD3DAddressMode(addressMode);

        mD3DSampler.Filter = d3dFilter;
        mD3DSampler.AddressU = d3dAddressMode;
        mD3DSampler.AddressV = d3dAddressMode;
        mD3DSampler.AddressW = d3dAddressMode;
        mD3DSampler.MipLODBias = 0;
        mD3DSampler.MaxAnisotropy = 0;
        mD3DSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        mD3DSampler.BorderColor[0] = 0;
        mD3DSampler.BorderColor[1] = 0;
        mD3DSampler.BorderColor[2] = 0;
        mD3DSampler.BorderColor[3] = 0;
        mD3DSampler.MinLOD = 0;
        mD3DSampler.MaxLOD = D3D12_FLOAT32_MAX;
    }

    D3D12_TEXTURE_ADDRESS_MODE Sampler::GetD3DAddressMode(AddressMode mode) const
    {
        switch (mode)
        {
        case HAL::Sampler::AddressMode::Clamp: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case HAL::Sampler::AddressMode::Wrap: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case HAL::Sampler::AddressMode::Mirror: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        default: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        }
    }

    D3D12_COMPARISON_FUNC Sampler::GetD3DComparisonFunc(Comparator comparator) const
    {
        switch (comparator)
        {
        case Comparator::Never: return D3D12_COMPARISON_FUNC_NEVER;
        case Comparator::Less: return D3D12_COMPARISON_FUNC_LESS;
        case Comparator::Equal: return D3D12_COMPARISON_FUNC_EQUAL;
        case Comparator::LessOrEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case Comparator::Greater: return D3D12_COMPARISON_FUNC_GREATER;
        case Comparator::NotEqual: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case Comparator::GreaterOrEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case Comparator::Always: return D3D12_COMPARISON_FUNC_ALWAYS;
        default: return D3D12_COMPARISON_FUNC_ALWAYS;
        }
    }

}
