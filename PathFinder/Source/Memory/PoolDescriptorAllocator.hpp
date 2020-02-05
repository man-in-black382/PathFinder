#pragma once

#include "Pool.hpp"
#include "Ring.hpp"

#include "../HardwareAbstractionLayer/DescriptorHeap.hpp"
#include "../HardwareAbstractionLayer/Buffer.hpp"
#include "../HardwareAbstractionLayer/Texture.hpp"

#include <memory>
#include <functional>

namespace Memory
{
   
    class PoolDescriptorAllocator
    {
    public:
        template <class DescriptorT>
        using DescriptorPtr = std::unique_ptr<DescriptorT, std::function<void(DescriptorT*)>>;

        using RTDescriptorPtr = DescriptorPtr<HAL::RTDescriptor>;
        using DSDescriptorPtr = DescriptorPtr<HAL::DSDescriptor>;
        using SRDescriptorPtr = DescriptorPtr<HAL::SRDescriptor>;
        using UADescriptorPtr = DescriptorPtr<HAL::UADescriptor>;
        using CBDescriptorPtr = DescriptorPtr<HAL::CBDescriptor>;

        RTDescriptorPtr AllocateRTDescriptor(const HAL::Texture* texture, std::optional<HAL::ColorFormat> shaderVisibleFormat = std::nullopt);
        DSDescriptorPtr AllocateDSDescriptor(const HAL::Texture* texture);
        SRDescriptorPtr AllocateSRDescriptor(const HAL::Texture* texture, std::optional<HAL::ColorFormat> shaderVisibleFormat = std::nullopt);
        UADescriptorPtr AllocateUADescriptor(const HAL::Texture* texture, std::optional<HAL::ColorFormat> shaderVisibleFormat = std::nullopt);

        SRDescriptorPtr AllocateSRDescriptor(const HAL::Buffer* buffer, uint64_t stride);
        UADescriptorPtr AllocateUADescriptor(const HAL::Buffer* buffer, uint64_t stride);
        CBDescriptorPtr AllocateCBDescriptor(const HAL::Buffer* buffer, uint64_t stride);
        
    private:
        void ValidateRTFormatsCompatibility(HAL::ResourceFormat::FormatVariant textureFormat, std::optional<HAL::ColorFormat> shaderVisibleFormat);
        void ValidateSRUAFormatsCompatibility(HAL::ResourceFormat::FormatVariant textureFormat, std::optional<HAL::ColorFormat> shaderVisibleFormat);
    };

}

