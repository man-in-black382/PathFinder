#include "GPUResourceProducer.hpp"

namespace Memory
{

    GPUResourceProducer::GPUResourceProducer(
        const HAL::Device* device,
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        ResourceStateTracker* stateTracker, 
        PoolDescriptorAllocator* descriptorAllocator,
        CopyRequestManager* copyRequestManager)
        : 
        mDevice{ device },
        mResourceAllocator{ resourceAllocator }, 
        mStateTracker{ stateTracker }, 
        mDescriptorAllocator{ descriptorAllocator },
        mCopyRequestManager{ copyRequestManager } {}

    GPUResourceProducer::TexturePtr GPUResourceProducer::NewTexture(const HAL::TextureProperties& properties)
    {
        CheckFrameValidity();

        Texture* texture = new Texture{ properties, mStateTracker, mResourceAllocator, mDescriptorAllocator, mCopyRequestManager };
        auto [iter, success] = mAllocatedResources.insert(texture);
        texture->BeginFrame(mFrameNumber);

        auto deallocationCallback = [this, iter](Texture* texture)
        {
            mAllocatedResources.erase(iter);
            delete texture;
        };

        return TexturePtr{ texture, deallocationCallback };
    }

    GPUResourceProducer::TexturePtr GPUResourceProducer::NewTexture(const HAL::TextureProperties& properties, const HAL::Heap& explicitHeap, uint64_t heapOffset)
    {
        CheckFrameValidity();

        Texture* texture = new Texture{
            properties, mStateTracker, mResourceAllocator, mDescriptorAllocator, 
            mCopyRequestManager, *mDevice, explicitHeap, heapOffset
        };

        auto [iter, success] = mAllocatedResources.insert(texture);
        texture->BeginFrame(mFrameNumber);

        auto deallocationCallback = [this, iter](Texture* texture)
        {
            mAllocatedResources.erase(iter);
            delete texture;
        };

        return TexturePtr{ texture, deallocationCallback };
    }

    GPUResourceProducer::TexturePtr GPUResourceProducer::NewTexture(HAL::Texture* existingTexture)
    {
        CheckFrameValidity();

        Texture* texture = new Texture{ mStateTracker, mResourceAllocator, mDescriptorAllocator, mCopyRequestManager, existingTexture };
        auto [iter, success] = mAllocatedResources.insert(texture);
        texture->BeginFrame(mFrameNumber);

        auto deallocationCallback = [this, iter](Texture* texture)
        {
            mAllocatedResources.erase(iter);
            delete texture;
        };

        return TexturePtr{ texture, deallocationCallback };
    }

    GPUResourceProducer::BufferPtr GPUResourceProducer::NewBuffer(const HAL::BufferProperties& properties, GPUResource::AccessStrategy accessStrategy)
    {
        CheckFrameValidity();

        Buffer* buffer = new Buffer{ properties, accessStrategy, mStateTracker, mResourceAllocator, mDescriptorAllocator, mCopyRequestManager };
        auto [iter, success] = mAllocatedResources.insert(buffer);
        buffer->BeginFrame(mFrameNumber);

        auto deallocationCallback = [this, iter](Buffer* buffer)
        {
            mAllocatedResources.erase(iter);
            delete buffer;
        };

        return BufferPtr{ buffer, deallocationCallback };
    }

    GPUResourceProducer::BufferPtr GPUResourceProducer::NewBuffer(const HAL::BufferProperties& properties, const HAL::Heap& explicitHeap, uint64_t heapOffset)
    {
        CheckFrameValidity();

        Buffer* buffer = new Buffer{
            properties, mStateTracker, mResourceAllocator,
            mDescriptorAllocator, mCopyRequestManager, *mDevice, explicitHeap, heapOffset
        };

        auto [iter, success] = mAllocatedResources.insert(buffer);
        buffer->BeginFrame(mFrameNumber);

        auto deallocationCallback = [this, iter](Buffer* buffer)
        {
            mAllocatedResources.erase(iter);
            delete buffer;
        };

        return BufferPtr{ buffer, deallocationCallback };
    }

    void GPUResourceProducer::BeginFrame(uint64_t frameNumber)
    {
        mFrameNumber = frameNumber;

        for (GPUResource* resource : mAllocatedResources)
        {
            resource->BeginFrame(frameNumber);
        }
    }

    void GPUResourceProducer::EndFrame(uint64_t frameNumber)
    {
        for (GPUResource* resource : mAllocatedResources)
        {
            resource->EndFrame(frameNumber);
        }
    }

    void GPUResourceProducer::CheckFrameValidity()
    {
        assert_format(mFrameNumber > 0, "Allocations cannot happen before first frame start");
    }

}
