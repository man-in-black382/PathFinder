#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/DescriptorHeap.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"
#include "../HardwareAbstractionLayer/TextureResource.hpp"
#include "../HardwareAbstractionLayer/BufferResource.hpp"

#include <unordered_map>

namespace PathFinder
{

    class ResourceDescriptorStorage
    {
    public:
        using ResourceName = Foundation::Name;
        
        ResourceDescriptorStorage(HAL::Device* device);

        const HAL::RTDescriptor* GetRTDescriptor(ResourceName resourceName, HAL::ResourceFormat::Color format) const;
        const HAL::DSDescriptor* GetDSDescriptor(ResourceName resourceName) const;
        const HAL::SRDescriptor* GetSRDescriptor(ResourceName resourceName, std::optional<HAL::ResourceFormat::Color> format = std::nullopt) const;
        const HAL::UADescriptor* GetUADescriptor(ResourceName resourceName, std::optional<HAL::ResourceFormat::Color> format = std::nullopt) const;
        const HAL::CBDescriptor* GetCBDescriptor(ResourceName resourceName) const;

        HAL::RTDescriptor EmplaceRTDescriptorIfNeeded(ResourceName resourceName, const HAL::ColorTexture& texture);
        HAL::RTDescriptor EmplaceRTDescriptorIfNeeded(ResourceName resourceName, const HAL::TypelessTexture& texture, HAL::ResourceFormat::Color format);

        HAL::DSDescriptor EmplaceDSDescriptorIfNeeded(ResourceName resourceName, const HAL::DepthStencilTexture& texture);

        HAL::RTDescriptor EmplaceSRDescriptorIfNeeded(ResourceName resourceName, const HAL::ColorTexture& texture);
        HAL::RTDescriptor EmplaceSRDescriptorIfNeeded(ResourceName resourceName, const HAL::TypelessTexture& texture, HAL::ResourceFormat::Color format);

        HAL::RTDescriptor EmplaceUADescriptorIfNeeded(ResourceName resourceName, const HAL::ColorTexture& texture);
        HAL::RTDescriptor EmplaceUADescriptorIfNeeded(ResourceName resourceName, const HAL::TypelessTexture& texture, HAL::ResourceFormat::Color format);

    private:
        using RTDescriptorMap = std::unordered_map<ResourceName, std::unordered_map<HAL::ResourceFormat::Color, HAL::RTDescriptor>>;
        using DSDescriptorMap = std::unordered_map<ResourceName, HAL::DSDescriptor>;
        using SRTextureDescriptorMap = std::unordered_map<ResourceName, std::unordered_map<HAL::ResourceFormat::Color, HAL::SRDescriptor>>;
        using UATextureDescriptorMap = std::unordered_map<ResourceName, std::unordered_map<HAL::ResourceFormat::Color, HAL::UADescriptor>>;
        using SRBufferDescriptorMap = std::unordered_map<ResourceName,  HAL::SRDescriptor>;
        using UABufferDescriptorMap = std::unordered_map<ResourceName, HAL::UADescriptor>;
        using CBDescriptorMap = std::unordered_map<ResourceName, HAL::CBDescriptor>;

        uint32_t mDescriptorHeapCapacity = 10000;

        HAL::RTDescriptorHeap mRTDescriptorHeap;
        HAL::DSDescriptorHeap mDSDescriptorHeap;
        HAL::CBSRUADescriptorHeap mCBSRUADescriptorHeap;
       
        RTDescriptorMap mRTDescriptorMap;
        DSDescriptorMap mDSDescriptorMap;
        SRTextureDescriptorMap mSRTextureDescriptorMap;
        UATextureDescriptorMap mUATextureDescriptorMap;
        SRBufferDescriptorMap mSRBufferDescriptorMap;
        UABufferDescriptorMap mUABufferDescriptorMap;
        CBDescriptorMap mCBDescriptorMap;
    };

}
