#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/DescriptorHeap.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"
#include "../HardwareAbstractionLayer/TextureResource.hpp"
#include "../HardwareAbstractionLayer/BufferResource.hpp"

#include <unordered_map>
#include <optional>

namespace PathFinder
{

    class ResourceDescriptorStorage
    {
    public:
        using ResourceName = Foundation::Name;
        
        ResourceDescriptorStorage(HAL::Device* device);

        const HAL::RTDescriptor* GetRTDescriptor(ResourceName resourceName, HAL::ResourceFormat::Color format);
        const HAL::DSDescriptor* GetDSDescriptor(ResourceName resourceName);
        const HAL::SRDescriptor* GetSRDescriptor(ResourceName resourceName, std::optional<HAL::ResourceFormat::Color> format = std::nullopt);
        const HAL::UADescriptor* GetUADescriptor(ResourceName resourceName, std::optional<HAL::ResourceFormat::Color> format = std::nullopt);
        const HAL::CBDescriptor* GetCBDescriptor(ResourceName resourceName);

        const HAL::RTDescriptor& EmplaceRTDescriptorIfNeeded(
            ResourceName resourceName, const HAL::TextureResource& texture, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat = std::nullopt
        );

        const HAL::DSDescriptor& EmplaceDSDescriptorIfNeeded(
            ResourceName resourceName, const HAL::TextureResource& texture
        );

        const HAL::SRDescriptor& EmplaceSRDescriptorIfNeeded(
            ResourceName resourceName, const HAL::TextureResource& texture, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat = std::nullopt
        );

        const HAL::UADescriptor& EmplaceUADescriptorIfNeeded(
            ResourceName resourceName, const HAL::TextureResource& texture, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat = std::nullopt
        );

    private:
        struct DSCBSet
        {
            const HAL::DSDescriptor* dsDescriptor = nullptr;
            const HAL::CBDescriptor* cbDescriptor = nullptr;
        };

        struct SRUASet
        {
            const HAL::SRDescriptor* srDescriptor = nullptr;
            const HAL::UADescriptor* uaDescriptor = nullptr;
        };

        struct RTSRUASet : SRUASet
        {
            const HAL::RTDescriptor* rtDescriptor = nullptr;
        };
        
        struct DescriptorSet
        {
            DSCBSet DSCB;
            RTSRUASet TypelessSRUA; // For typeless buffers (StructuredBuffer for example)
            std::unordered_map<HAL::ResourceFormat::Color, RTSRUASet> RTSRUA; // For typed resources, such as textures and typed buffers
        };

        void ValidateRTFormatsCompatibility(HAL::ResourceFormat::FormatVariant textureFormat, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat);
        void ValidateSRUAFormatsCompatibility(HAL::ResourceFormat::FormatVariant textureFormat, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat);

        const DSCBSet& GetDSCBSet(ResourceName name);
        const SRUASet& GetTypelessSRUASet(ResourceName name);
        const RTSRUASet& GetRTSRUASet(ResourceName name, HAL::ResourceFormat::Color format);

        uint32_t mDescriptorHeapCapacity = 10000;

        HAL::RTDescriptorHeap mRTDescriptorHeap;
        HAL::DSDescriptorHeap mDSDescriptorHeap;
        HAL::CBSRUADescriptorHeap mCBSRUADescriptorHeap;
       
        std::unordered_map<ResourceName, DescriptorSet> mDescriptors;
    };

}
