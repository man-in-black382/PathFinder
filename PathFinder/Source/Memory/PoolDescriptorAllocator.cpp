#include "PoolDescriptorAllocator.hpp"

#include "../Foundation/Assert.hpp"

namespace Memory
{

    PoolDescriptorAllocator::PoolDescriptorAllocator(const HAL::Device* device, uint8_t simultaneousFramesInFlight)
        : mCBSRUADescriptorHeap{ device, mDescriptorRangeCapacity, mDescriptorRangeCapacity, mDescriptorRangeCapacity },
        mRTDescriptorHeap{ device, mDescriptorRangeCapacity },
        mDSDescriptorHeap{ device, mDescriptorRangeCapacity },
        mSamplerDescriptorHeap{ device, mDescriptorRangeCapacity },
        mRingFrameTracker{ simultaneousFramesInFlight },
        mRTPool{ 1, mDescriptorRangeCapacity },
        mDSPool{ 1, mDescriptorRangeCapacity },
        mCBPool{ 1, mDescriptorRangeCapacity },
        mSRPool{ 1, mDescriptorRangeCapacity },
        mUAPool{ 1, mDescriptorRangeCapacity },
        mSamplerPool{ 1, mDescriptorRangeCapacity }
    {
        // Just allocate pools and descriptor heaps with fixed size and hope nobody needs more than N descriptors of each type
        // TODO: add grow logic similar to how stl vector grows if more descriptors is actually needed

        mRingFrameTracker.SetDeallocationCallback([this](const Ring::FrameTailAttributes& frameAttributes)
        {
            auto frameIndex = frameAttributes.Tail - frameAttributes.Size;
            ExecutePendingDeallocations(frameIndex);
        });

        mPendingDeallocations.resize(simultaneousFramesInFlight);
    }

    PoolDescriptorAllocator::RTDescriptorPtr PoolDescriptorAllocator::AllocateRTDescriptor(const HAL::Texture& texture, uint8_t mipLevel, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {
        ValidateRTFormatsCompatibility(texture.Format(), shaderVisibleFormat);

        auto slot = mRTPool.Allocate();
        auto descriptor = mRTDescriptorHeap.EmplaceRTDescriptor(slot.MemoryOffset, texture, mipLevel, shaderVisibleFormat);
        auto& allocation = mAllocatedRTDescriptors.emplace_back(descriptor, slot);
        auto deallocationCallback = [this, &allocation](HAL::RTDescriptor* descriptor) {
            mPendingDeallocations[mCurrentFrameIndex].emplace_back(allocation.Slot, &mRTPool);
        };

        return RTDescriptorPtr(&allocation.Descriptor, deallocationCallback);
    }

    PoolDescriptorAllocator::DSDescriptorPtr PoolDescriptorAllocator::AllocateDSDescriptor(const HAL::Texture& texture)
    {
        assert_format(std::holds_alternative<HAL::DepthStencilFormat>(texture.Format()), "Texture is not of depth-stencil format");

        auto slot = mDSPool.Allocate();
        auto descriptor = mDSDescriptorHeap.EmplaceDSDescriptor(slot.MemoryOffset, texture);
        auto& allocation = mAllocatedDSDescriptors.emplace_back(descriptor, slot);
        auto deallocationCallback = [this, &allocation](HAL::DSDescriptor* descriptor) {
            mPendingDeallocations[mCurrentFrameIndex].emplace_back(allocation.Slot, &mDSPool);
        };

        return DSDescriptorPtr(&allocation.Descriptor, deallocationCallback);
    }

    PoolDescriptorAllocator::SRDescriptorPtr PoolDescriptorAllocator::AllocateSRDescriptor(const HAL::Texture& texture, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {
        ValidateSRUAFormatsCompatibility(texture.Format(), shaderVisibleFormat);

        auto slot = mSRPool.Allocate();
        auto descriptor = mCBSRUADescriptorHeap.EmplaceSRDescriptor(slot.MemoryOffset, texture, shaderVisibleFormat);
        auto& allocation = mAllocatedSRDescriptors.emplace_back(descriptor, slot);
        auto deallocationCallback = [this, &allocation](HAL::SRDescriptor* descriptor) {
            mPendingDeallocations[mCurrentFrameIndex].emplace_back(allocation.Slot, &mSRPool);
        };

        return SRDescriptorPtr(&allocation.Descriptor, deallocationCallback);
    }

    PoolDescriptorAllocator::UADescriptorPtr PoolDescriptorAllocator::AllocateUADescriptor(const HAL::Texture& texture, uint8_t mipLevel, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {
        ValidateSRUAFormatsCompatibility(texture.Format(), shaderVisibleFormat);

        auto slot = mUAPool.Allocate();
        auto descriptor = mCBSRUADescriptorHeap.EmplaceUADescriptor(slot.MemoryOffset, texture, mipLevel, shaderVisibleFormat);
        auto& allocation = mAllocatedUADescriptors.emplace_back(descriptor, slot);
        auto deallocationCallback = [this, &allocation](HAL::UADescriptor* descriptor) {
            mPendingDeallocations[mCurrentFrameIndex].emplace_back(allocation.Slot, &mUAPool);
        };

        return UADescriptorPtr(&allocation.Descriptor, deallocationCallback);
    }

    PoolDescriptorAllocator::SRDescriptorPtr PoolDescriptorAllocator::AllocateSRDescriptor(const HAL::Buffer& buffer, uint64_t stride)
    {
        auto slot = mSRPool.Allocate();
        auto descriptor = mCBSRUADescriptorHeap.EmplaceSRDescriptor(slot.MemoryOffset, buffer, stride);
        auto& allocation = mAllocatedSRDescriptors.emplace_back(descriptor, slot);
        auto deallocationCallback = [this, &allocation](HAL::SRDescriptor* descriptor) {
            mPendingDeallocations[mCurrentFrameIndex].emplace_back(allocation.Slot, &mSRPool);
        };

        return SRDescriptorPtr(&allocation.Descriptor, deallocationCallback);
    }

    PoolDescriptorAllocator::UADescriptorPtr PoolDescriptorAllocator::AllocateUADescriptor(const HAL::Buffer& buffer, uint64_t stride)
    {
        auto slot = mUAPool.Allocate();
        auto descriptor = mCBSRUADescriptorHeap.EmplaceUADescriptor(slot.MemoryOffset, buffer, stride);
        auto& allocation = mAllocatedUADescriptors.emplace_back(descriptor, slot);
        auto deallocationCallback = [this, &allocation](HAL::UADescriptor* descriptor) {
            mPendingDeallocations[mCurrentFrameIndex].emplace_back(allocation.Slot, &mUAPool);
        };

        return UADescriptorPtr(&allocation.Descriptor, deallocationCallback);
    }

    PoolDescriptorAllocator::CBDescriptorPtr PoolDescriptorAllocator::AllocateCBDescriptor(const HAL::Buffer& buffer, uint64_t stride)
    {
        auto slot = mCBPool.Allocate();
        auto descriptor = mCBSRUADescriptorHeap.EmplaceCBDescriptor(slot.MemoryOffset, buffer, stride);
        auto& allocation = mAllocatedCBDescriptors.emplace_back(descriptor, slot);
        auto deallocationCallback = [this, &allocation](HAL::CBDescriptor* descriptor) {
            mPendingDeallocations[mCurrentFrameIndex].emplace_back(allocation.Slot, &mCBPool);
        };

        return CBDescriptorPtr(&allocation.Descriptor, deallocationCallback);
    }

    PoolDescriptorAllocator::SamplerDescriptorPtr PoolDescriptorAllocator::AllocateSamplerDescriptor(const HAL::Sampler& sampler)
    {
        auto slot = mSamplerPool.Allocate();
        auto descriptor = mSamplerDescriptorHeap.EmplaceSamplerDescriptor(slot.MemoryOffset, sampler);
        auto& allocation = mAllocatedSamplerDescriptors.emplace_back(descriptor, slot);
        auto deallocationCallback = [this, &allocation](HAL::SamplerDescriptor* descriptor) {
            mPendingDeallocations[mCurrentFrameIndex].emplace_back(allocation.Slot, &mSamplerPool);
        };

        return SamplerDescriptorPtr(&allocation.Descriptor, deallocationCallback);
    }

    void PoolDescriptorAllocator::BeginFrame(uint64_t frameNumber)
    {
        mCurrentFrameIndex = mRingFrameTracker.Allocate(1);
        mRingFrameTracker.FinishCurrentFrame(frameNumber);
    }

    void PoolDescriptorAllocator::EndFrame(uint64_t frameNumber)
    {
        mRingFrameTracker.ReleaseCompletedFrames(frameNumber);
    }

    void PoolDescriptorAllocator::ExecutePendingDeallocations(uint64_t frameIndex)
    {
        for (Deallocation& deallocation : mPendingDeallocations[frameIndex])
        {
            deallocation.PoolPtr->Deallocate(deallocation.Slot);
        }
        mPendingDeallocations[frameIndex].clear();
    }

    void PoolDescriptorAllocator::ValidateRTFormatsCompatibility(
        HAL::FormatVariant textureFormat, std::optional<HAL::ColorFormat> shaderVisibleFormat)
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
        HAL::FormatVariant textureFormat, std::optional<HAL::ColorFormat> shaderVisibleFormat)
    {
        assert_format(!shaderVisibleFormat || std::holds_alternative<HAL::TypelessColorFormat>(textureFormat), "Format redefinition for typed texture");
    }

}
