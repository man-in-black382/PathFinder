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

        const HAL::RTDescriptor* GetRTDescriptor(const HAL::Resource* resource, std::optional<HAL::ResourceFormat::Color> format = std::nullopt);
        const HAL::DSDescriptor* GetDSDescriptor(const HAL::Resource* resource);
        const HAL::SRDescriptor* GetSRDescriptor(const HAL::Resource* resource, std::optional<HAL::ResourceFormat::Color> format = std::nullopt);
        const HAL::UADescriptor* GetUADescriptor(const HAL::Resource* resource, std::optional<HAL::ResourceFormat::Color> format = std::nullopt);
        const HAL::CBDescriptor* GetCBDescriptor(const HAL::Resource* resource);

        const HAL::RTDescriptor& EmplaceRTDescriptorIfNeeded(const HAL::TextureResource* texture, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat = std::nullopt);
        const HAL::DSDescriptor& EmplaceDSDescriptorIfNeeded(const HAL::TextureResource* texture);
        const HAL::SRDescriptor& EmplaceSRDescriptorIfNeeded(const HAL::TextureResource* texture, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat = std::nullopt);
        const HAL::UADescriptor& EmplaceUADescriptorIfNeeded(const HAL::TextureResource* texture, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat = std::nullopt);

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
            RTSRUASet ImplicitlyTypedRTSRUA; // For typeless buffers (StructuredBuffer for example)
            std::unordered_map<HAL::ResourceFormat::Color, RTSRUASet> ExplicitlyTypedRTSRUA; // For typed resources, such as textures and typed buffers
        };

        void ValidateRTFormatsCompatibility(HAL::ResourceFormat::FormatVariant textureFormat, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat);
        void ValidateSRUAFormatsCompatibility(HAL::ResourceFormat::FormatVariant textureFormat, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat);

        const DSCBSet& GetDSCBSet(const HAL::Resource* resource);
        const RTSRUASet& GetRTSRUASet(const HAL::Resource* resource, std::optional<HAL::ResourceFormat::Color> format);

        uint32_t mDescriptorHeapCapacity = 10000;

        HAL::RTDescriptorHeap mRTDescriptorHeap;
        HAL::DSDescriptorHeap mDSDescriptorHeap;
        HAL::CBSRUADescriptorHeap mCBSRUADescriptorHeap;
       
        std::unordered_map<const HAL::Resource*, DescriptorSet> mDescriptors;

    public:
        const HAL::RTDescriptorHeap& RTDescriptorHeap() const { return mRTDescriptorHeap; }
        const HAL::DSDescriptorHeap& DSDescriptorHeap() const { return mDSDescriptorHeap; }
        const HAL::CBSRUADescriptorHeap& CBSRUADescriptorHeap() const { return mCBSRUADescriptorHeap; }
    };

}
