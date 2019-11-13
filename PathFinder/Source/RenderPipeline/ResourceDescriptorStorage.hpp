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

        const HAL::RTDescriptor* GetRTDescriptor(const HAL::Resource* resource, std::optional<HAL::ResourceFormat::Color> format = std::nullopt) const;
        const HAL::DSDescriptor* GetDSDescriptor(const HAL::Resource* resource) const;
        const HAL::SRDescriptor* GetSRDescriptor(const HAL::Resource* resource, std::optional<HAL::ResourceFormat::Color> format = std::nullopt) const;
        const HAL::UADescriptor* GetUADescriptor(const HAL::Resource* resource, std::optional<HAL::ResourceFormat::Color> format = std::nullopt) const;
        const HAL::CBDescriptor* GetCBDescriptor(const HAL::Resource* resource) const;

        const HAL::RTDescriptor& EmplaceRTDescriptorIfNeeded(const HAL::TextureResource* texture, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat = std::nullopt);
        const HAL::DSDescriptor& EmplaceDSDescriptorIfNeeded(const HAL::TextureResource* texture);
        const HAL::SRDescriptor& EmplaceSRDescriptorIfNeeded(const HAL::TextureResource* texture, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat = std::nullopt);
        const HAL::UADescriptor& EmplaceUADescriptorIfNeeded(const HAL::TextureResource* texture, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat = std::nullopt);

        template <class T> const HAL::SRDescriptor& EmplaceSRDescriptorIfNeeded(const HAL::BufferResource<T>* buffer, std::optional<uint64_t> explicitStride = std::nullopt);
        template <class T> const HAL::UADescriptor& EmplaceUADescriptorIfNeeded(const HAL::BufferResource<T>* buffer, std::optional<uint64_t> explicitStride = std::nullopt);
        template <class T> const HAL::CBDescriptor& EmplaceCBDescriptorIfNeeded(const HAL::BufferResource<T>* buffer, std::optional<uint64_t> explicitStride = std::nullopt);

    private:
        struct DSCBSet
        {
            const HAL::DSDescriptor* DSDescriptor = nullptr;
            const HAL::CBDescriptor* CBDescriptor = nullptr;
        };

        struct SRUASet
        {
            const HAL::SRDescriptor* SRDescriptor = nullptr;
            const HAL::UADescriptor* UADescriptor = nullptr;
        };

        struct RTSRUASet : SRUASet
        {
            const HAL::RTDescriptor* RTDescriptor = nullptr;
        };
        
        struct DescriptorSet
        {
            DSCBSet DSCB;
            RTSRUASet ImplicitlyTypedRTSRUA; // For typeless buffers (StructuredBuffer, ByteBuffer for example)
            std::unordered_map<HAL::ResourceFormat::Color, RTSRUASet> ExplicitlyTypedRTSRUA; // For typed resources, such as textures and typed buffers

            const RTSRUASet* GetExplicitlyTypedRTSRUA(HAL::ResourceFormat::Color format) const;
        };

        void ValidateRTFormatsCompatibility(HAL::ResourceFormat::FormatVariant textureFormat, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat);
        void ValidateSRUAFormatsCompatibility(HAL::ResourceFormat::FormatVariant textureFormat, std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat);

        const DSCBSet* GetDSCBSet(const HAL::Resource* resource) const;
        const RTSRUASet* GetRTSRUASet(const HAL::Resource* resource, std::optional<HAL::ResourceFormat::Color> format) const;

        uint32_t mDescriptorHeapRangeCapacity = 1000;

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

#include "ResourceDescriptorStorage.inl"