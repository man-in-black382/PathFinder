#include "ResourceDescriptorStorage.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    ResourceDescriptorStorage::ResourceDescriptorStorage(HAL::Device* device)
        : mRTDescriptorHeap{ device, mDescriptorHeapCapacity },
        mDSDescriptorHeap{ device, mDescriptorHeapCapacity },
        mCBSRUADescriptorHeap{ device, mDescriptorHeapCapacity } {}

    const HAL::RTDescriptor* ResourceDescriptorStorage::GetRTDescriptor(ResourceName resourceName, std::optional<HAL::ResourceFormat::Color> format)
    {
        return GetRTSRUASet(resourceName, format).rtDescriptor;
    }

    const HAL::DSDescriptor* ResourceDescriptorStorage::GetDSDescriptor(ResourceName resourceName)
    {
        return GetDSCBSet(resourceName).dsDescriptor;
    }

    const HAL::SRDescriptor* ResourceDescriptorStorage::GetSRDescriptor(ResourceName resourceName, std::optional<HAL::ResourceFormat::Color> format)
    {
        return GetRTSRUASet(resourceName, format).srDescriptor;
    }

    const HAL::UADescriptor* ResourceDescriptorStorage::GetUADescriptor(ResourceName resourceName, std::optional<HAL::ResourceFormat::Color> format)
    {
        return GetRTSRUASet(resourceName, format).uaDescriptor;
    }

    const HAL::CBDescriptor* ResourceDescriptorStorage::GetCBDescriptor(ResourceName resourceName)
    {
        return GetDSCBSet(resourceName).cbDescriptor;
    }

    const HAL::RTDescriptor& ResourceDescriptorStorage::EmplaceRTDescriptorIfNeeded(
        ResourceName resourceName, const HAL::TextureResource& texture, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat)
    {
        ValidateRTFormatsCompatibility(texture.Format(), shaderVisibleFormat);

        if (auto descriptor = GetRTDescriptor(resourceName, shaderVisibleFormat)) return *descriptor;

        const HAL::RTDescriptor& descriptor = mRTDescriptorHeap.EmplaceRTDescriptor(texture, shaderVisibleFormat);

        if (shaderVisibleFormat)
        {
            mDescriptors[resourceName].ExplicitlyTypedRTSRUA[*shaderVisibleFormat].rtDescriptor = &descriptor;
        }
        else {
            mDescriptors[resourceName].ImplicitlyTypedRTSRUA.rtDescriptor = &descriptor;
        }

        return descriptor;
    }

    const HAL::DSDescriptor& ResourceDescriptorStorage::EmplaceDSDescriptorIfNeeded(
        ResourceName resourceName, const HAL::TextureResource& texture)
    {
        assert_format(std::holds_alternative<HAL::ResourceFormat::DepthStencil>(texture.Format()), "Texture is not of depth-stencil format");

        if (auto descriptor = GetDSDescriptor(resourceName)) return *descriptor;

        const HAL::DSDescriptor& descriptor = mDSDescriptorHeap.EmplaceDSDescriptor(texture);
        mDescriptors[resourceName].DSCB.dsDescriptor = &descriptor;
        return descriptor;
    }

    const HAL::SRDescriptor& ResourceDescriptorStorage::EmplaceSRDescriptorIfNeeded(
        ResourceName resourceName, const HAL::TextureResource& texture, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat)
    {
        ValidateSRUAFormatsCompatibility(texture.Format(), shaderVisibleFormat);

        if (auto descriptor = GetSRDescriptor(resourceName, shaderVisibleFormat)) return *descriptor;

        const HAL::SRDescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceSRDescriptor(texture, shaderVisibleFormat);

        if (shaderVisibleFormat)
        {
            mDescriptors[resourceName].ExplicitlyTypedRTSRUA[*shaderVisibleFormat].srDescriptor = &descriptor;
        } 
        else {
            mDescriptors[resourceName].ImplicitlyTypedRTSRUA.srDescriptor = &descriptor;
        }
        
        return descriptor;
    }

    const HAL::UADescriptor& ResourceDescriptorStorage::EmplaceUADescriptorIfNeeded(
        ResourceName resourceName, const HAL::TextureResource& texture, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat)
    {
        ValidateSRUAFormatsCompatibility(texture.Format(), shaderVisibleFormat);

        if (auto descriptor = GetUADescriptor(resourceName, shaderVisibleFormat)) return *descriptor;

        const HAL::UADescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceUADescriptor(texture, shaderVisibleFormat);

        if (shaderVisibleFormat)
        {
            mDescriptors[resourceName].ExplicitlyTypedRTSRUA[*shaderVisibleFormat].uaDescriptor = &descriptor;
        }
        else {
            mDescriptors[resourceName].ImplicitlyTypedRTSRUA.uaDescriptor = &descriptor;
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

    const ResourceDescriptorStorage::DSCBSet& ResourceDescriptorStorage::GetDSCBSet(ResourceName name)
    {
        return mDescriptors[name].DSCB;
    }

    const ResourceDescriptorStorage::RTSRUASet& ResourceDescriptorStorage::GetRTSRUASet(ResourceName name, std::optional<HAL::ResourceFormat::Color> format)
    {
        if (format)
        {
            return mDescriptors[name].ExplicitlyTypedRTSRUA[*format];
        } 
        else {
            return mDescriptors[name].ImplicitlyTypedRTSRUA;
        }
    }

}
