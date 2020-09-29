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
        Texture* texture = new Texture{ properties, mStateTracker, mResourceAllocator, mDescriptorAllocator, mCopyRequestManager };
        auto [iter, success] = mAllocatedResources.insert(texture);

        auto deallocationCallback = [this, iter](Texture* texture)
        {
            mAllocatedResources.erase(iter);
            delete texture;
        };

        return TexturePtr{ texture, deallocationCallback };
    }

    GPUResourceProducer::TexturePtr GPUResourceProducer::NewTexture(const HAL::TextureProperties& properties, const HAL::Heap& explicitHeap, uint64_t heapOffset)
    {
        Texture* texture = new Texture{
            properties, mStateTracker, mResourceAllocator, mDescriptorAllocator, 
            mCopyRequestManager, *mDevice, explicitHeap, heapOffset
        };

        auto [iter, success] = mAllocatedResources.insert(texture);

        auto deallocationCallback = [this, iter](Texture* texture)
        {
            mAllocatedResources.erase(iter);
            delete texture;
        };

        return TexturePtr{ texture, deallocationCallback };
    }

    GPUResourceProducer::TexturePtr GPUResourceProducer::NewTexture(HAL::Texture* existingTexture)
    {
        Texture* texture = new Texture{ mStateTracker, mResourceAllocator, mDescriptorAllocator, mCopyRequestManager, existingTexture };
        auto [iter, success] = mAllocatedResources.insert(texture);

        auto deallocationCallback = [this, iter](Texture* texture)
        {
            mAllocatedResources.erase(iter);
            delete texture;
        };

        return TexturePtr{ texture, deallocationCallback };
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

}
