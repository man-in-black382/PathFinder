#include "PoolDescriptorAllocator.hpp"

#include "../Foundation/Assert.hpp"

namespace Memory
{

    PoolDescriptorAllocator::PoolDescriptorAllocator(const HAL::Device* device, uint8_t simultaneousFramesInFlight)
        : mCBSRUADescriptorHeap{ device, mDescriptorRangeCapacity, mDescriptorRangeCapacity, mDescriptorRangeCapacity },
        mRTDescriptorHeap{ device, mDescriptorRangeCapacity },
        mDSDescriptorHeap{ device, mDescriptorRangeCapacity },
        mRingFrameTracker{ simultaneousFramesInFlight },
        mRTPool{ 1, mDescriptorRangeCapacity },
        mDSPool{ 1, mDescriptorRangeCapacity },
        mCBPool{ 1, mDescriptorRangeCapacity },
        mSRPool{ 1, mDescriptorRangeCapacity },
        mUAPool{ 1, mDescriptorRangeCapacity }
    {
        // Just allocate pools and descriptor heaps with fixed size and hope nobody needs more than N descriptors of each type
        // TODO: add grow logic similar to how stl vector grows if more descriptors is actually needed
    }

    PoolDescriptorAllocator::RTDescriptorPtr PoolDescriptorAllocator::AllocateRTDescriptor(const HAL::Texture* texture, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {

    }

    PoolDescriptorAllocator::DSDescriptorPtr PoolDescriptorAllocator::AllocateDSDescriptor(const HAL::Texture* texture)
    {

    }

    PoolDescriptorAllocator::SRDescriptorPtr PoolDescriptorAllocator::AllocateSRDescriptor(const HAL::Texture* texture, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {
        ValidateSRUAFormatsCompatibility(texture->Format(), shaderVisibleFormat);

        auto slot = mSRPool.Allocate();
        auto descriptor = mCBSRUADescriptorHeap.EmplaceSRDescriptor(slot.MemoryOffset, *texture, shaderVisibleFormat);

        auto descriptorIt = mAllocatedSRDescriptors.emplace_back(descriptor, slot);

        auto deallocationCallback = [this, descriptorIt](HAL::SRDescriptor* descriptor)
        {
            mSRPool.Deallocate(descriptorIt.Slot);
        };

        return SRDescriptorPtr(&descriptorIt.Descriptor, deallocationCallback);
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
