#include "ResourceDescriptorStorage.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceDescriptorStorage::ResourceDescriptorStorage(HAL::Device* device)
        : mRTDescriptorHeap{ device, mDescriptorHeapCapacity },
        mDSDescriptorHeap{ device, mDescriptorHeapCapacity },
        mCBSRUADescriptorHeap{ device, mDescriptorHeapCapacity } {}

    const HAL::RTDescriptor* ResourceDescriptorStorage::GetRTDescriptor(const HAL::Resource* resource, std::optional<HAL::ResourceFormat::Color> format)
    {
        return GetRTSRUASet(resource, format).rtDescriptor;
    }

    const HAL::DSDescriptor* ResourceDescriptorStorage::GetDSDescriptor(const HAL::Resource* resource)
    {
        return GetDSCBSet(resource).dsDescriptor;
    }

    const HAL::SRDescriptor* ResourceDescriptorStorage::GetSRDescriptor(const HAL::Resource* resource, std::optional<HAL::ResourceFormat::Color> format)
    {
        return GetRTSRUASet(resource, format).srDescriptor;
    }

    const HAL::UADescriptor* ResourceDescriptorStorage::GetUADescriptor(const HAL::Resource* resource, std::optional<HAL::ResourceFormat::Color> format)
    {
        return GetRTSRUASet(resource, format).uaDescriptor;
    }

    const HAL::CBDescriptor* ResourceDescriptorStorage::GetCBDescriptor(const HAL::Resource* resource)
    {
        return GetDSCBSet(resource).cbDescriptor;
    }

    const HAL::RTDescriptor& ResourceDescriptorStorage::EmplaceRTDescriptorIfNeeded(const HAL::TextureResource* texture, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat)
    {
        ValidateRTFormatsCompatibility(texture->Format(), shaderVisibleFormat);

        if (auto descriptor = GetRTDescriptor(texture, shaderVisibleFormat)) return *descriptor;

        const HAL::RTDescriptor& descriptor = mRTDescriptorHeap.EmplaceRTDescriptor(*texture, shaderVisibleFormat);

        if (shaderVisibleFormat)
        {
            mDescriptors[texture].ExplicitlyTypedRTSRUA[*shaderVisibleFormat].rtDescriptor = &descriptor;
        }
        else {
            mDescriptors[texture].ImplicitlyTypedRTSRUA.rtDescriptor = &descriptor;
        }

        return descriptor;
    }

    const HAL::DSDescriptor& ResourceDescriptorStorage::EmplaceDSDescriptorIfNeeded(const HAL::TextureResource* texture)
    {
        assert_format(std::holds_alternative<HAL::ResourceFormat::DepthStencil>(texture->Format()), "Texture is not of depth-stencil format");

        if (auto descriptor = GetDSDescriptor(texture)) return *descriptor;

        const HAL::DSDescriptor& descriptor = mDSDescriptorHeap.EmplaceDSDescriptor(*texture);
        mDescriptors[texture].DSCB.dsDescriptor = &descriptor;
        return descriptor;
    }

    const HAL::SRDescriptor& ResourceDescriptorStorage::EmplaceSRDescriptorIfNeeded(const HAL::TextureResource* texture, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat)
    {
        ValidateSRUAFormatsCompatibility(texture->Format(), shaderVisibleFormat);

        if (auto descriptor = GetSRDescriptor(texture, shaderVisibleFormat)) return *descriptor;

        const HAL::SRDescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceSRDescriptor(*texture, shaderVisibleFormat);

        if (shaderVisibleFormat)
        {
            mDescriptors[texture].ExplicitlyTypedRTSRUA[*shaderVisibleFormat].srDescriptor = &descriptor;
        } 
        else {
            mDescriptors[texture].ImplicitlyTypedRTSRUA.srDescriptor = &descriptor;
        }
        
        return descriptor;
    }

    const HAL::UADescriptor& ResourceDescriptorStorage::EmplaceUADescriptorIfNeeded(
        const HAL::TextureResource* texture, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat)
    {
        ValidateSRUAFormatsCompatibility(texture->Format(), shaderVisibleFormat);

        if (auto descriptor = GetUADescriptor(texture, shaderVisibleFormat)) return *descriptor;

        const HAL::UADescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceUADescriptor(*texture, shaderVisibleFormat);

        if (shaderVisibleFormat)
        {
            mDescriptors[texture].ExplicitlyTypedRTSRUA[*shaderVisibleFormat].uaDescriptor = &descriptor;
        }
        else {
            mDescriptors[texture].ImplicitlyTypedRTSRUA.uaDescriptor = &descriptor;
        }

        return descriptor;
    }

    void ResourceDescriptorStorage::ValidateRTFormatsCompatibility(
        HAL::ResourceFormat::FormatVariant textureFormat, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat)
    {
        if (shaderVisibleFormat)
        {
            assert_format(std::holds_alternative<HAL::ResourceFormat::TypelessColor>(textureFormat), "Format redefinition for texture that has it's own format");
        }
        else {
            assert_format(std::holds_alternative<HAL::ResourceFormat::Color>(textureFormat), "Texture format is not suited for render targets");
        }
    }

    void ResourceDescriptorStorage::ValidateSRUAFormatsCompatibility(
        HAL::ResourceFormat::FormatVariant textureFormat, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat)
    {
        assert_format(!shaderVisibleFormat || std::holds_alternative<HAL::ResourceFormat::TypelessColor>(textureFormat), "Format redefinition for typed texture");
    }

    const PathFinder::ResourceDescriptorStorage::DSCBSet& ResourceDescriptorStorage::GetDSCBSet(const HAL::Resource* resource)
    {
        return mDescriptors[resource].DSCB;
    }

    const ResourceDescriptorStorage::RTSRUASet& ResourceDescriptorStorage::GetRTSRUASet(const HAL::Resource* resource, std::optional<HAL::ResourceFormat::Color> format)
    {
        if (format)
        {
            return mDescriptors[resource].ExplicitlyTypedRTSRUA[*format];
        } 
        else {
            return mDescriptors[resource].ImplicitlyTypedRTSRUA;
        }
    }

}
