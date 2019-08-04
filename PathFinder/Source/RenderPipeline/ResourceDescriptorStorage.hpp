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

        const HAL::RTDescriptor* TryGetRTDescriptor(ResourceName resourceName, HAL::ResourceFormat::Color format) const;
        const HAL::DSDescriptor* TryGetDSDescriptor(ResourceName resourceName) const;

        HAL::RTDescriptor EmplaceRTDescriptorIfNeeded(ResourceName resourceName, const HAL::ColorTextureResource& texture);
        HAL::RTDescriptor EmplaceRTDescriptorIfNeeded(ResourceName resourceName, const HAL::TypelessTextureResource& texture, HAL::ResourceFormat::Color format);
        HAL::DSDescriptor EmplaceDSDescriptorIfNeeded(ResourceName resourceName, const HAL::DepthStencilTextureResource& texture);

    private:
        using RTDescriptorMap = std::unordered_map<ResourceName, std::unordered_map<HAL::ResourceFormat::Color, HAL::RTDescriptor>>;
        using DSDescriptorMap = std::unordered_map<ResourceName, HAL::DSDescriptor>;
        /* using SRUACBDescriptorMap = std::unordered_map<ResourceName, std::unordered_map<HAL::ResourceFormat::UnderlyingType, SRDescriptor>;
         using UADescriptorMap = std::unordered_map<ResourceName, std::unordered_map<HAL::ResourceFormat::UnderlyingType, UADescriptor>;
         using CBDescriptorMap = std::unordered_map<ResourceName, std::unordered_map<HAL::ResourceFormat::UnderlyingType, CBDescriptor>;*/

        uint32_t mDescriptorHeapCapacity = 1000000;

        HAL::RTDescriptorHeap mRTDescriptorHeap;
        HAL::DSDescriptorHeap mDSDescriptorHeap;
        HAL::CBSRUADescriptorHeap mCBSRUADescriptorHeap;
       
        RTDescriptorMap mRTDescriptorMap;
        DSDescriptorMap mDSDescriptorMap;
    };

}
