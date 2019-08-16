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
        inline const D3D12_ROOT_PARAMETER& D3DParameter() const { return mParameter; }
    };

    

    class RootDescriptorTableParameter : public RootParameter {
    public:
        RootDescriptorTableParameter();
        RootDescriptorTableParameter(const RootDescriptorTableParameter& that);
        RootDescriptorTableParameter(RootDescriptorTableParameter&& that);
        ~RootDescriptorTableParameter() = default;

        RootDescriptorTableParameter& operator=(const RootDescriptorTableParameter& that);
        RootDescriptorTableParameter& operator=(RootDescriptorTableParameter&& that);

        void AddDescriptorRange(const RootDescriprorTableRange& range);

    private:
        std::vector<D3D12_DESCRIPTOR_RANGE> mRanges;
    };



    class RootConstantsParameter : public RootParameter {
    public:
        //RootConstantsParameter();
        ~RootConstantsParameter() = default;
    };


    class RootDescriptorParameter : public RootParameter {
    public:
        RootDescriptorParameter(uint32_t shaderRegister, uint32_t registerSpace);
        ~RootDescriptorParameter() = default;
    };

}

