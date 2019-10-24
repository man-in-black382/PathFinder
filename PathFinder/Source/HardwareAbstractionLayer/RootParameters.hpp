#pragma once

#include <d3d12.h>
#include <cstdint>
#include <vector>

#include "DescriptorTableRange.hpp"
#include "ShaderRegister.hpp"

namespace HAL
{

    class RootParameter {
    public:
        struct LocationInSignature
        {
            uint16_t BaseRegister = 0;
            uint16_t RegisterSpace = 0;
            ShaderRegister RegisterType;
        };

        struct LocationHasher { size_t operator()(const LocationInSignature& key) const; };
        struct LocationEquality { size_t operator()(const LocationInSignature& left, const LocationInSignature& right) const; };

        RootParameter(D3D12_ROOT_PARAMETER_TYPE type);
        virtual ~RootParameter() = 0;

    protected:
        D3D12_ROOT_PARAMETER mParameter;

        void AddSignatureLocation(const LocationInSignature& location);

    private:
        std::vector<LocationInSignature> mSignatureLocations;

    public:
        inline const D3D12_ROOT_PARAMETER& D3DParameter() const { return mParameter; }
        inline const std::vector<LocationInSignature>& SignatureLocations() const { return mSignatureLocations; }
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
        // TODO: Implement
        //RootConstantsParameter();
        ~RootConstantsParameter() = default;
    };


    class RootDescriptorParameter : public RootParameter {
    public:
        RootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE type, uint16_t shaderRegister, uint16_t registerSpace, ShaderRegister registerType);
    };

    class RootConstantBufferParameter : public RootDescriptorParameter {
    public:
        RootConstantBufferParameter(uint16_t shaderRegister, uint16_t registerSpace);
        ~RootConstantBufferParameter() = default;
    };

    class RootShaderResourceParameter : public RootDescriptorParameter {
    public:
        RootShaderResourceParameter(uint16_t shaderRegister, uint16_t registerSpace);
        ~RootShaderResourceParameter() = default;
    };

    class RootUnorderedAccessParameter : public RootDescriptorParameter {
    public:
        RootUnorderedAccessParameter(uint16_t shaderRegister, uint16_t registerSpace);
        ~RootUnorderedAccessParameter() = default;
    };

}

