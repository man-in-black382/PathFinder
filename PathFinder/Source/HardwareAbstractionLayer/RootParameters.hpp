#pragma once

#include <d3d12.h>
#include <cstdint>
#include <vector>

#include "DescriptorTableRange.hpp"

namespace HAL
{

    class RootParameter {
    public:
        RootParameter(D3D12_ROOT_PARAMETER_TYPE type);
        virtual ~RootParameter() = 0;

    protected:
        D3D12_ROOT_PARAMETER mParameter;

    public:
        inline const auto& D3DParameter() const { return mParameter; }
    };

    
    class RootDescriptorTableParameter : public RootParameter {
    public:
        RootDescriptorTableParameter();
        ~RootDescriptorTableParameter() = default;

        void AddDescriptorRange(const CBSRUADescriptorTableRange& range);
        void AddDescriptorRange(const SamplerDescriptorTableRange& range);

    private:
        std::vector<D3D12_DESCRIPTOR_RANGE> mRanges;
    };


    class RootConstantsParameter : public RootParameter {
    public:
        //RootConstantsParameter();
        ~RootConstantsParameter() = default;

    private:
        D3D12_ROOT_CONSTANTS mConstants;
    };


    class RootDescriptorParameter : public RootParameter {
    public:
        //RootDescriptorParameter();
        ~RootDescriptorParameter() = default;

    private:
        D3D12_ROOT_DESCRIPTOR mDescriptor;
    };

}

