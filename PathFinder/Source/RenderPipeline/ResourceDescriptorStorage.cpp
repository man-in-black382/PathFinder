#include "ResourceDescriptorStorage.hpp"

namespace PathFinder
{

    ResourceDescriptorStorage::ResourceDescriptorStorage(HAL::Device* device)
        : mRTDescriptorHeap{ device, mDescriptorHeapCapacity },
        mDSDescriptorHeap{ device, mDescriptorHeapCapacity },
        mCBSRUADescriptorHeap{ device, mDescriptorHeapCapacity } {}

    HAL::RTDescriptor* ResourceDescriptorStorage::TryGetRTDescriptor(ResourceName resourceName, HAL::ResourceFormat::Color format)
    {
        auto mapIt = mRTDescriptorMap.find(resourceName);
        if (mapIt == mRTDescriptorMap.end()) return nullptr;

        auto& nestedMap = mapIt->second;
        auto nestedMapIt = nestedMap.find(format);
        if (nestedMapIt == nestedMap.end()) return nullptr;

        return &nestedMapIt->second;
    }

    HAL::DSDescriptor* ResourceDescriptorStorage::TryGetDSDescriptor(ResourceName resourceName)
    {
        auto mapIt = mDSDescriptorMap.find(resourceName);
        if (mapIt == mDSDescriptorMap.end()) return nullptr;

        return &mapIt->second;
    }

    HAL::RTDescriptor ResourceDescriptorStorage::EmplaceRTDescriptorIfNeeded(ResourceName resourceName, const HAL::ColorTextureResource& texture)
    {
        if (HAL::RTDescriptor* descriptor = TryGetRTDescriptor(resourceName, texture.DataFormat())) return *descriptor;

        HAL::RTDescriptor descriptor = mRTDescriptorHeap.EmplaceDescriptorForResource(texture);
        mRTDescriptorMap[resourceName].emplace(texture.DataFormat(), descriptor);
        return descriptor;
    }

    HAL::RTDescriptor ResourceDescriptorStorage::EmplaceRTDescriptorIfNeeded(ResourceName resourceName, const HAL::TypelessTextureResource& texture, HAL::ResourceFormat::Color format)
    {
        if (HAL::RTDescriptor* descriptor = TryGetRTDescriptor(resourceName, format)) return *descriptor;

        HAL::RTDescriptor descriptor = mRTDescriptorHeap.EmplaceDescriptorForResource(texture, format);
        mRTDescriptorMap[resourceName].emplace(format, descriptor);
        return descriptor;
    }

    HAL::DSDescriptor ResourceDescriptorStorage::EmplaceDSDescriptorIfNeeded(ResourceName resourceName, const HAL::DepthStencilTextureResource& texture)
    {
        if (HAL::DSDescriptor* descriptor = TryGetDSDescriptor(resourceName)) return *descriptor;

        HAL::DSDescriptor descriptor = mDSDescriptorHeap.EmplaceDescriptorForResource(texture);
        mDSDescriptorMap.emplace(resourceName, descriptor);
        return descriptor;
    }

}
