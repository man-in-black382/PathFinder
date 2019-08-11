#include "ResourceDescriptorStorage.hpp"

namespace PathFinder
{

    ResourceDescriptorStorage::ResourceDescriptorStorage(HAL::Device* device)
        : mRTDescriptorHeap{ device, mDescriptorHeapCapacity },
        mDSDescriptorHeap{ device, mDescriptorHeapCapacity },
        mCBSRUADescriptorHeap{ device, mDescriptorHeapCapacity } {}

    const HAL::RTDescriptor* ResourceDescriptorStorage::GetRTDescriptor(ResourceName resourceName, HAL::ResourceFormat::Color format) const
    {
        auto mapIt = mRTDescriptorMap.find(resourceName);

        if (mapIt == mRTDescriptorMap.end()) {
            return nullptr;
        }

        auto& nestedMap = mapIt->second;
        auto nestedMapIt = nestedMap.find(format);

        if (nestedMapIt == nestedMap.end()) {
            return nullptr;
        }

        return &nestedMapIt->second;
    }

    const HAL::DSDescriptor* ResourceDescriptorStorage::GetDSDescriptor(ResourceName resourceName) const
    {
        auto mapIt = mDSDescriptorMap.find(resourceName);

        if (mapIt == mDSDescriptorMap.end()) {
            return nullptr;
        }

        return &mapIt->second;
    }

    const HAL::SRDescriptor* ResourceDescriptorStorage::GetSRDescriptor(ResourceName resourceName, std::optional<HAL::ResourceFormat::Color> format) const
    {
        if (format)
        {
            auto outerMapIt = mSRTextureDescriptorMap.find(resourceName);
            if (outerMapIt != mSRTextureDescriptorMap.end())
            {
                auto innerMapIt = outerMapIt->second.find(*format);
                if (innerMapIt != outerMapIt->second.end())
                {
                    return &innerMapIt->second;
                }
            }
        }
        else {
            auto mapIt = mSRBufferDescriptorMap.find(resourceName);
            if (mapIt != mSRBufferDescriptorMap.end()) {
                return &mapIt->second;
            }
        }

        return nullptr;
    }

    const HAL::UADescriptor* ResourceDescriptorStorage::GetUADescriptor(ResourceName resourceName, std::optional<HAL::ResourceFormat::Color> format) const
    {
        if (format)
        {
            auto outerMapIt = mUATextureDescriptorMap.find(resourceName);
            if (outerMapIt != mUATextureDescriptorMap.end())
            {
                auto innerMapIt = outerMapIt->second.find(*format);
                if (innerMapIt != outerMapIt->second.end())
                {
                    return &innerMapIt->second;
                }
            }
        }
        else {
            auto mapIt = mUABufferDescriptorMap.find(resourceName);
            if (mapIt != mUABufferDescriptorMap.end()) {
                return &mapIt->second;
            }
        }

        return nullptr;
    }

    const HAL::CBDescriptor* ResourceDescriptorStorage::GetCBDescriptor(ResourceName resourceName) const
    {
        auto mapIt = mCBDescriptorMap.find(resourceName);
        if (mapIt == mCBDescriptorMap.end()) return nullptr;

        return &mapIt->second;
    }

    HAL::RTDescriptor ResourceDescriptorStorage::EmplaceRTDescriptorIfNeeded(ResourceName resourceName, const HAL::ColorTexture& texture)
    {
        if (auto descriptor = GetRTDescriptor(resourceName, texture.DataFormat())) return *descriptor;

        HAL::RTDescriptor descriptor = mRTDescriptorHeap.EmplaceDescriptorForTexture(texture);
        mRTDescriptorMap[resourceName].emplace(texture.DataFormat(), descriptor);
        return descriptor;
    }

    HAL::RTDescriptor ResourceDescriptorStorage::EmplaceRTDescriptorIfNeeded(ResourceName resourceName, const HAL::TypelessTexture& texture, HAL::ResourceFormat::Color format)
    {
        if (auto descriptor = GetRTDescriptor(resourceName, format)) return *descriptor;

        HAL::RTDescriptor descriptor = mRTDescriptorHeap.EmplaceDescriptorForTexture(texture, format);
        mRTDescriptorMap[resourceName].emplace(format, descriptor);
        return descriptor;
    }

    HAL::DSDescriptor ResourceDescriptorStorage::EmplaceDSDescriptorIfNeeded(ResourceName resourceName, const HAL::DepthStencilTexture& texture)
    {
        if (auto descriptor = GetDSDescriptor(resourceName)) return *descriptor;

        HAL::DSDescriptor descriptor = mDSDescriptorHeap.EmplaceDescriptorForResource(texture);
        mDSDescriptorMap.emplace(resourceName, descriptor);
        return descriptor;
    }

    HAL::RTDescriptor ResourceDescriptorStorage::EmplaceSRDescriptorIfNeeded(ResourceName resourceName, const HAL::ColorTexture& texture)
    {

    }

    HAL::RTDescriptor ResourceDescriptorStorage::EmplaceSRDescriptorIfNeeded(ResourceName resourceName, const HAL::TypelessTexture& texture, HAL::ResourceFormat::Color format)
    {

    }

    HAL::RTDescriptor ResourceDescriptorStorage::EmplaceUADescriptorIfNeeded(ResourceName resourceName, const HAL::ColorTexture& texture)
    {

    }

    HAL::RTDescriptor ResourceDescriptorStorage::EmplaceUADescriptorIfNeeded(ResourceName resourceName, const HAL::TypelessTexture& texture, HAL::ResourceFormat::Color format)
    {

    }

}
