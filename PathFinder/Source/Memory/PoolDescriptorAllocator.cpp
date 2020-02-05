#include "PoolDescriptorAllocator.hpp"

#include "../Foundation/Assert.hpp"

namespace Memory
{

    PoolDescriptorAllocator::RTDescriptorPtr PoolDescriptorAllocator::AllocateRTDescriptor(const HAL::Texture* texture, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {

    }

    PoolDescriptorAllocator::DSDescriptorPtr PoolDescriptorAllocator::AllocateDSDescriptor(const HAL::Texture* texture)
    {

    }

    PoolDescriptorAllocator::SRDescriptorPtr PoolDescriptorAllocator::AllocateSRDescriptor(const HAL::Texture* texture, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {

    }

    PoolDescriptorAllocator::UADescriptorPtr PoolDescriptorAllocator::AllocateUADescriptor(const HAL::Texture* texture, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {

    }

    PoolDescriptorAllocator::SRDescriptorPtr PoolDescriptorAllocator::AllocateSRDescriptor(const HAL::Buffer* buffer, uint64_t stride)
    {

    }

    PoolDescriptorAllocator::UADescriptorPtr PoolDescriptorAllocator::AllocateUADescriptor(const HAL::Buffer* buffer, uint64_t stride)
    {

    }

    PoolDescriptorAllocator::CBDescriptorPtr PoolDescriptorAllocator::AllocateCBDescriptor(const HAL::Buffer* buffer, uint64_t stride)
    {

    }

    void PoolDescriptorAllocator::ValidateRTFormatsCompatibility(
        HAL::ResourceFormat::FormatVariant textureFormat, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {
        if (shaderVisibleFormat)
        {
            assert_format(std::holds_alternative<HAL::TypelessColorFormat>(textureFormat), "Format redefinition for texture that has it's own format");
        }
        else {
            assert_format(std::holds_alternative<HAL::ColorFormat>(textureFormat), "Texture format is not suited for render targets");
        }
    }

    void PoolDescriptorAllocator::ValidateSRUAFormatsCompatibility(
        HAL::ResourceFormat::FormatVariant textureFormat, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {
        assert_format(!shaderVisibleFormat || std::holds_alternative<HAL::TypelessColorFormat>(textureFormat), "Format redefinition for typed texture");
    }

}
