#include "ResourceDescriptorStorage.hpp"

namespace PathFinder
{

    ResourceDescriptorStorage::ResourceDescriptorStorage(HAL::Device* device)
        : mRTDescriptorHeap{ device, mDescriptorHeapCapacity },
        mDSDescriptorHeap{ device, mDescriptorHeapCapacity },
        mCBSRUADescriptorHeap{ device, mDescriptorHeapCapacity } {}

    const HAL::RTDescriptor* ResourceDescriptorStorage::GetRTDescriptor(ResourceName resourceName, HAL::ResourceFormat::Color format)
    {
        return GetRTSRUASet(resourceName, format).rtDescriptor;
    }

    const HAL::DSDescriptor* ResourceDescriptorStorage::GetDSDescriptor(ResourceName resourceName)
    {
        return GetDSCBSet(resourceName).dsDescriptor;
    }

    const HAL::SRDescriptor* ResourceDescriptorStorage::GetSRDescriptor(ResourceName resourceName, std::optional<HAL::ResourceFormat::Color> format)
    {
        if (format)
        {
            return GetRTSRUASet(resourceName, *format).srDescriptor;
        }
        else {
            return GetRTSRUASet(resourceName).srDescriptor;
        }
    }

    const HAL::UADescriptor* ResourceDescriptorStorage::GetUADescriptor(ResourceName resourceName, std::optional<HAL::ResourceFormat::Color> format)
    {
        if (format)
        {
            return GetRTSRUASet(resourceName, *format).uaDescriptor;
        }
        else {
            return GetRTSRUASet(resourceName).uaDescriptor;
        }
    }

    const HAL::CBDescriptor* ResourceDescriptorStorage::GetCBDescriptor(ResourceName resourceName)
    {
        return GetDSCBSet(resourceName).cbDescriptor;
    }

    const HAL::RTDescriptor& ResourceDescriptorStorage::EmplaceRTDescriptorIfNeeded(ResourceName resourceName, const HAL::ColorTexture& texture)
    {
        if (auto descriptor = GetRTDescriptor(resourceName, texture.DataFormat())) return *descriptor;

        const HAL::RTDescriptor& descriptor = mRTDescriptorHeap.EmplaceDescriptorForTexture(texture);
        mDescriptors[resourceName].RTSRUA[texture.DataFormat()].rtDescriptor = &descriptor;
        return descriptor;
    }

    const HAL::RTDescriptor& ResourceDescriptorStorage::EmplaceRTDescriptorIfNeeded(ResourceName resourceName, const HAL::TypelessTexture& texture, HAL::ResourceFormat::Color format)
    {
        if (auto descriptor = GetRTDescriptor(resourceName, format)) return *descriptor;

        const HAL::RTDescriptor& descriptor = mRTDescriptorHeap.EmplaceDescriptorForTexture(texture, format);
        mDescriptors[resourceName].RTSRUA[format].rtDescriptor = &descriptor;
        return descriptor;
    }

    const HAL::DSDescriptor& ResourceDescriptorStorage::EmplaceDSDescriptorIfNeeded(ResourceName resourceName, const HAL::DepthStencilTexture& texture)
    {
        if (auto descriptor = GetDSDescriptor(resourceName)) return *descriptor;

        const HAL::DSDescriptor& descriptor = mDSDescriptorHeap.EmplaceDescriptorForResource(texture);
        mDescriptors[resourceName].DSCB.dsDescriptor = &descriptor;
        return descriptor;
    }

    const HAL::SRDescriptor& ResourceDescriptorStorage::EmplaceSRDescriptorIfNeeded(ResourceName resourceName, const HAL::ColorTexture& texture)
    {
        if (auto descriptor = GetSRDescriptor(resourceName, texture.DataFormat())) return *descriptor;

        const HAL::SRDescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceDescriptorForTexture(texture);
        mDescriptors[resourceName].RTSRUA[texture.DataFormat()].srDescriptor = &descriptor;
        return descriptor;
    }

    const HAL::SRDescriptor& ResourceDescriptorStorage::EmplaceSRDescriptorIfNeeded(ResourceName resourceName, const HAL::TypelessTexture& texture, HAL::ResourceFormat::Color format)
    {
        if (auto descriptor = GetSRDescriptor(resourceName, format)) return *descriptor;

        const HAL::SRDescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceDescriptorForTexture(texture, format);
        mDescriptors[resourceName].RTSRUA[format].srDescriptor = &descriptor;
        return descriptor;
    }

    const HAL::UADescriptor& ResourceDescriptorStorage::EmplaceUADescriptorIfNeeded(ResourceName resourceName, const HAL::ColorTexture& texture)
    {
        if (auto descriptor = GetUADescriptor(resourceName, texture.DataFormat())) return *descriptor;

        const HAL::UADescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceDescriptorForUnorderedAccessTexture(texture);
        mDescriptors[resourceName].RTSRUA[texture.DataFormat()].uaDescriptor = &descriptor;
        return descriptor;
    }

    const HAL::UADescriptor& ResourceDescriptorStorage::EmplaceUADescriptorIfNeeded(ResourceName resourceName, const HAL::TypelessTexture& texture, HAL::ResourceFormat::Color format)
    {
        if (auto descriptor = GetUADescriptor(resourceName, format)) return *descriptor;

        const HAL::UADescriptor& descriptor = mCBSRUADescriptorHeap.EmplaceDescriptorForUnorderedAccessTexture(texture, format);
        mDescriptors[resourceName].RTSRUA[format].uaDescriptor = &descriptor;
        return descriptor;
    }

    const ResourceDescriptorStorage::DSCBSet& ResourceDescriptorStorage::GetDSCBSet(ResourceName name)
    {
        return mDescriptors[name].DSCB;
    }

    const ResourceDescriptorStorage::RTSRUASet& ResourceDescriptorStorage::GetRTSRUASet(ResourceName name)
    {
        return mDescriptors[name].TypelessRTSRUA;
    }

    const ResourceDescriptorStorage::RTSRUASet& ResourceDescriptorStorage::GetRTSRUASet(ResourceName name, HAL::ResourceFormat::Color format)
    {
        return mDescriptors[name].RTSRUA[format];
    }

}
