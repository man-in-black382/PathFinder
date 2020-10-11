#include "RootParameters.hpp"
#include "Utils.h"



namespace HAL
{

    RootParameter::RootParameter(D3D12_ROOT_PARAMETER_TYPE type)
    {
        mParameter.ParameterType = type;
        mParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }

    RootParameter::~RootParameter() {}

    void RootParameter::AddSignatureLocation(const LocationInSignature& location)
    {
        mSignatureLocations.push_back(location);
    }



    RootDescriptorTableParameter::RootDescriptorTableParameter()
        : RootParameter(D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {}

    RootDescriptorTableParameter::RootDescriptorTableParameter(const RootDescriptorTableParameter& that) : RootParameter(that)
    {
        *this = that;
    }

    RootDescriptorTableParameter::RootDescriptorTableParameter(RootDescriptorTableParameter&& that) : RootParameter(std::move(that))
    {
        *this = std::move(that);
    }

    RootDescriptorTableParameter& RootDescriptorTableParameter::operator=(const RootDescriptorTableParameter& that)
    {
        if (this == &that) return *this;

        mRanges = that.mRanges;
        mParameter.DescriptorTable.NumDescriptorRanges = (UINT)mRanges.size();
        mParameter.DescriptorTable.pDescriptorRanges = mRanges.data();

        return *this;
    }

    RootDescriptorTableParameter& RootDescriptorTableParameter::operator=(RootDescriptorTableParameter&& that)
    {
        if (this == &that) return *this;

        mRanges = std::move(that.mRanges);
        mParameter.DescriptorTable.NumDescriptorRanges = (UINT)mRanges.size();
        mParameter.DescriptorTable.pDescriptorRanges = mRanges.data();

        return *this;
    }

    void RootDescriptorTableParameter::AddDescriptorRange(const RootDescriprorTableRange& range)
    {
        assert_format(mRanges.empty() || mRanges.back().NumDescriptors != RootDescriprorTableRange::UnboundedRangeSize,
            "Cannot insert any ranges in a table after an unbounded range");

        AddSignatureLocation({ (uint16_t)range.D3DRange().BaseShaderRegister, (uint16_t)range.D3DRange().RegisterSpace, range.ShaderRegisterType() });

        mRanges.push_back(range.D3DRange());
        mParameter.DescriptorTable.pDescriptorRanges = &mRanges[0];
        mParameter.DescriptorTable.NumDescriptorRanges = (UINT)mRanges.size();
    }



    RootConstantsParameter::RootConstantsParameter(uint16_t numberOf32BitValues, uint16_t shaderRegister, uint16_t registerSpace)
        : RootParameter(D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS)
    {
        AddSignatureLocation({ shaderRegister, registerSpace, HAL::ShaderRegister::ConstantBuffer });
        mParameter.Constants = { shaderRegister, registerSpace, numberOf32BitValues };
    }



    RootDescriptorParameter::RootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE type, uint16_t shaderRegister, uint16_t registerSpace, ShaderRegister registerType)
        : RootParameter(type)
    {
        AddSignatureLocation({ shaderRegister, registerSpace, registerType });
        mParameter.Descriptor = { shaderRegister, registerSpace };
    }

    RootConstantBufferParameter::RootConstantBufferParameter(uint16_t shaderRegister, uint16_t registerSpace)
        : RootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_CBV, shaderRegister, registerSpace, ShaderRegister::ConstantBuffer) {}

    RootShaderResourceParameter::RootShaderResourceParameter(uint16_t shaderRegister, uint16_t registerSpace)
        : RootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_SRV, shaderRegister, registerSpace, ShaderRegister::ShaderResource) {}

    RootUnorderedAccessParameter::RootUnorderedAccessParameter(uint16_t shaderRegister, uint16_t registerSpace)
        : RootDescriptorParameter(D3D12_ROOT_PARAMETER_TYPE_UAV, shaderRegister, registerSpace, ShaderRegister::UnorderedAccess) {}



    size_t RootParameter::LocationHasher::operator()(const LocationInSignature& key) const
    {
        size_t hashValue = 0;
        hashValue |= key.BaseRegister;
        hashValue <<= std::numeric_limits<decltype(key.BaseRegister)>::digits;
        hashValue |= key.RegisterSpace;
        hashValue <<= std::numeric_limits<decltype(key.RegisterSpace)>::digits;
        hashValue |= std::underlying_type_t<ShaderRegister>(key.RegisterType);
        return hashValue;
    }

    size_t RootParameter::LocationEquality::operator()(const LocationInSignature& left, const LocationInSignature& right) const
    {
        return left.BaseRegister == right.BaseRegister && 
            left.RegisterSpace == right.RegisterSpace && 
            left.RegisterType == right.RegisterType;
    }

}

