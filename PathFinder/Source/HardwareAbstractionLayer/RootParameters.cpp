#include "RootParameters.hpp"
#include "Utils.h"

#include "../Foundation/Assert.hpp"

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

    RootDescriptorTableParameter::RootDescriptorTableParameter(const RootDescriptorTableParameter& that)
        : RootDescriptorTableParameter()
    {
        *this = that;
    }

    RootDescriptorTableParameter::RootDescriptorTableParameter(RootDescriptorTableParameter&& that) 
        : RootDescriptorTableParameter()
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

        mRanges.push_back(range.D3DRange());
        mParameter.DescriptorTable.pDescriptorRanges = &mRanges[0];
        mParameter.DescriptorTable.NumDescriptorRanges = (UINT)mRanges.size();

        AddSignatureLocation({ range.D3DRange().BaseShaderRegister, range.D3DRange().RegisterSpace });
    }



    RootDescriptorParameter::RootDescriptorParameter(uint32_t shaderRegister, uint32_t registerSpace)
        : RootParameter(D3D12_ROOT_PARAMETER_TYPE_CBV)
    {
        mParameter.Descriptor = { shaderRegister, registerSpace };
        AddSignatureLocation({ shaderRegister, registerSpace });
    }

    size_t RootParameter::LocationHasher::operator()(const LocationInSignature& key) const
    {
        size_t hashValue = 0;
        hashValue |= key.BaseRegister;
        hashValue <<= std::numeric_limits<decltype(key.BaseRegister)>::digits;
        hashValue |= key.RegisterSpace;
        return hashValue;
    }

    size_t RootParameter::LocationEquality::operator()(const LocationInSignature& left, const LocationInSignature& right) const
    {
        return left.BaseRegister == right.BaseRegister && left.RegisterSpace == right.RegisterSpace;
    }

}

