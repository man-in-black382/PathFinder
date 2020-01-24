#include "ResourceDescriptorStorage.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceDescriptorStorage::ResourceDescriptorStorage(HAL::Device* device)
        : mRTDescriptorHeap{ device, mDescriptorHeapRangeCapacity },
        mDSDescriptorHeap{ device, mDescriptorHeapRangeCapacity },
        mCBSRUADescriptorHeap{ device, mDescriptorHeapRangeCapacity } {}

    const HAL::RTDescriptor* ResourceDescriptorStorage::GetRTDescriptor(const HAL::Resource* resource, std::optional<HAL::ColorFormat> format) const
    {
        auto descriptorSet = GetRTSRUASet(resource, format);
        return descriptorSet ? descriptorSet->RTDescriptor : nullptr;
    }

    const HAL::DSDescriptor* ResourceDescriptorStorage::GetDSDescriptor(const HAL::Resource* resource) const
    {
        auto descriptorSet = GetDSCBSet(resource);
        return descriptorSet ? descriptorSet->DSDescriptor : nullptr;
    }

    const HAL::SRDescriptor* ResourceDescriptorStorage::GetSRDescriptor(const HAL::Resource* resource, std::optional<HAL::ColorFormat> format) const
    {
        auto descriptorSet = GetRTSRUASet(resource, format);
        return descriptorSet ? descriptorSet->SRDescriptor : nullptr;
    }

    const HAL::UADescriptor* ResourceDescriptorStorage::GetUADescriptor(const HAL::Resource* resource, std::optional<HAL::ColorFormat> format) const
    {
        auto descriptorSet = GetRTSRUASet(resource, format);
        return descriptorSet ? descriptorSet->UADescriptor : nullptr;
    }

    const HAL::CBDescriptor* ResourceDescriptorStorage::GetCBDescriptor(const HAL::Resource* resource) const
    {
        auto descriptorSet = GetDSCBSet(resource);
        return descriptorSet ? descriptorSet->CBDescriptor : nullptr;
    }

    const HAL::RTDescriptor& ResourceDescriptorStorage::EmplaceRTDescriptorIfNeeded(const HAL::Texture* texture, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {
        ValidateRTFormatsCompatibility(texture->Format(), shaderVisibleFormat);

        if (auto descriptor = GetRTDescriptor(texture, shaderVisibleFormat)) return *descriptor;

        const HAL::RTDescriptor& descriptor = mRTDescriptorHeap.EmplaceRTDescriptor(*texture, shaderVisibleFormat);

        if (shaderVisibleFormat)
        {
            mDescriptors[texture].ExplicitlyTypedRTSRUA[*shaderVisibleFormat].RTDescriptor = &descriptor;
        }
        else {
            mDescriptors[texture].ImplicitlyTypedRTSRUA.RTDescriptor = &descriptor;
        }

        return descriptor;
    }

    const HAL::DSDescriptor& ResourceDescriptorStorage::EmplaceDSDescriptorIfNeeded(const HAL::Texture* texture)
    {
        assert_format(std::holds_alternative<HAL::DepthStencilFormat>(texture->Format()), "Texture is not of depth-stencil format");

        if (auto descriptor = GetDSDescriptor(texture)) return *descriptor;

        const HAL::DSDescriptor& descriptor = mDSDescriptorHeap.EmplaceDSDescriptor(*texture);
        mDescriptors[texture].DSCB.DSDescriptor = &descriptor;
        return descriptor;
    }

    const HAL::SRDescriptor& ResourceDescriptorStorage::EmplaceSRDescriptorIfNeeded(const HAL::Texture* texture, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {
        ValidateSRUAFormatsCompatibility(texture->Format(), shaderVisibleFormat);

        if (auto descriptor = GetSRDescriptor(texture, shaderVisibleFormat)) return *descriptor;

        const HAL::SRDescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceSRDescriptor(*texture, shaderVisibleFormat);

        if (shaderVisibleFormat)
        {
            mDescriptors[texture].ExplicitlyTypedRTSRUA[*shaderVisibleFormat].SRDescriptor = &descriptor;
        }
        else {
            mDescriptors[texture].ImplicitlyTypedRTSRUA.SRDescriptor = &descriptor;
        }

        return descriptor;
    }

    const HAL::UADescriptor& ResourceDescriptorStorage::EmplaceUADescriptorIfNeeded(
        const HAL::Texture* texture, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {
        ValidateSRUAFormatsCompatibility(texture->Format(), shaderVisibleFormat);

        if (auto descriptor = GetUADescriptor(texture, shaderVisibleFormat)) return *descriptor;

        const HAL::UADescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceUADescriptor(*texture, shaderVisibleFormat);

        if (shaderVisibleFormat)
        {
            mDescriptors[texture].ExplicitlyTypedRTSRUA[*shaderVisibleFormat].UADescriptor = &descriptor;
        }
        else {
            mDescriptors[texture].ImplicitlyTypedRTSRUA.UADescriptor = &descriptor;
        }

        return descriptor;
    }

    const HAL::SRDescriptor& ResourceDescriptorStorage::EmplaceSRDescriptorIfNeeded(const HAL::Buffer* buffer, uint64_t stride)
    {
        if (auto descriptor = GetSRDescriptor(buffer)) return *descriptor;

        const HAL::SRDescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceSRDescriptor(*buffer, stride);
        mDescriptors[buffer].ImplicitlyTypedRTSRUA.SRDescriptor = &descriptor;

        return descriptor;
    }

    const HAL::UADescriptor& ResourceDescriptorStorage::EmplaceUADescriptorIfNeeded(const HAL::Buffer* buffer, uint64_t stride)
    {
        if (auto descriptor = GetUADescriptor(buffer)) return *descriptor;

        const HAL::UADescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceUADescriptor(*buffer, stride);
        mDescriptors[buffer].ImplicitlyTypedRTSRUA.UADescriptor = &descriptor;

        return descriptor;
    }

    const HAL::CBDescriptor& ResourceDescriptorStorage::EmplaceCBDescriptorIfNeeded(const HAL::Buffer* buffer, uint64_t stride)
    {
        if (auto descriptor = GetCBDescriptor(buffer)) return *descriptor;

        const HAL::CBDescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceCBDescriptor(*buffer, stride);
        mDescriptors[buffer].DSCB.CBDescriptor = &descriptor;

        return descriptor;
    }

    void ResourceDescriptorStorage::ValidateRTFormatsCompatibility(
        HAL::ResourceFormat::FormatVariant textureFormat, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {
        if (shaderVisibleFormat)
        {
            assert_format(std::holds_alternative<HAL::TypelessColorFormat>(textureFormat), "Format redefinition for texture that has it's own format");
        }
        else {
            assert_format(std::holds_alternative<HAL::ColorFormat>(textureFormat), "Texture format is not suited for render targets");
        }
    }

    void ResourceDescriptorStorage::ValidateSRUAFormatsCompatibility(
        HAL::ResourceFormat::FormatVariant textureFormat, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {
        assert_format(!shaderVisibleFormat || std::holds_alternative<HAL::TypelessColorFormat>(textureFormat), "Format redefinition for typed texture");
    }

    const ResourceDescriptorStorage::DSCBSet* ResourceDescriptorStorage::GetDSCBSet(const HAL::Resource* resource) const
    {
        auto it = mDescriptors.find(resource);
        return it != mDescriptors.end() ? &it->second.DSCB : nullptr;
    }

    const ResourceDescriptorStorage::RTSRUASet* ResourceDescriptorStorage::GetRTSRUASet(const HAL::Resource* resource, std::optional<HAL::ColorFormat> format) const
    {
        auto it = mDescriptors.find(resource);

        if (format)
        {
            return it != mDescriptors.end() ? it->second.GetExplicitlyTypedRTSRUA(*format) : nullptr;
        }
        else {
            return it != mDescriptors.end() ? &it->second.ImplicitlyTypedRTSRUA : nullptr;
        }
    }

    const ResourceDescriptorStorage::RTSRUASet* ResourceDescriptorStorage::DescriptorSet::GetExplicitlyTypedRTSRUA(HAL::ColorFormat format) const
    {
        auto it = ExplicitlyTypedRTSRUA.find(format);
        return it != ExplicitlyTypedRTSRUA.end() ? &it->second : nullptr;
    }

}
