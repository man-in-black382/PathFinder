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

    void ResourceDescriptorStorage::EmplaceRTDescriptorIfNeeded(ResourceName resourceName, const HAL::ColorTextureResource& texture)
    {
        if (TryGetRTDescriptor(resourceName, texture.DataFormat())) return;

        HAL::RTDescriptor descriptor = mRTDescriptorHeap.EmplaceDescriptorForResource(texture);
        mRTDescriptorMap[resourceName].emplace(texture.DataFormat(), descriptor);
    }

    void ResourceDescriptorStorage::EmplaceRTDescriptorIfNeeded(ResourceName resourceName, const HAL::TypelessTextureResource& texture, HAL::ResourceFormat::Color format)
    {
        if (TryGetRTDescriptor(resourceName, format)) return;

        HAL::RTDescriptor descriptor = mRTDescriptorHeap.EmplaceDescriptorForResource(texture, format);
        mRTDescriptorMap[resourceName].emplace(format, descriptor);
    }

    void ResourceDescriptorStorage::EmplaceDSDescriptorIfNeeded(ResourceName resourceName, const HAL::DepthStencilTextureResource& texture)
    {
        if (TryGetDSDescriptor(resourceName)) return;

        HAL::DSDescriptor descriptor = mDSDescriptorHeap.EmplaceDescriptorForResource(texture);
        mDSDescriptorMap.emplace(resourceName, descriptor);
    }

}
