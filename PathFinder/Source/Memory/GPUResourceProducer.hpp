#pragma once

#include "SegregatedPoolsResourceAllocator.hpp"
#include "ResourceStateTracker.hpp"
#include "PoolDescriptorAllocator.hpp"
#include "CopyRequestManager.hpp"
#include "Buffer.hpp"
#include "Texture.hpp"

#include <unordered_set>

namespace Memory
{

    class GPUResourceProducer
    {
    public:
        using BufferPtr = std::unique_ptr<Buffer, std::function<void(Buffer*)>>;
        using TexturePtr = std::unique_ptr<Texture, std::function<void(Texture*)>>;

        GPUResourceProducer(
            const HAL::Device* device, 
            SegregatedPoolsResourceAllocator* resourceAllocator,
            ResourceStateTracker* stateTracker,
            PoolDescriptorAllocator* descriptorAllocator,
            CopyRequestManager* copyRequestManager
        );

        BufferPtr NewBuffer(const HAL::BufferProperties& properties, GPUResource::AccessStrategy accessStrategy = GPUResource::AccessStrategy::Automatic);
        TexturePtr NewTexture(const HAL::TextureProperties& properties);

        BufferPtr NewBuffer(const HAL::BufferProperties& properties, const HAL::Heap& explicitHeap, uint64_t heapOffset);
        TexturePtr NewTexture(const HAL::TextureProperties& properties, const HAL::Heap& explicitHeap, uint64_t heapOffset);

        TexturePtr NewTexture(HAL::Texture* existingTexture);
        
        void BeginFrame(uint64_t frameNumber);
        void EndFrame(uint64_t frameNumber);

    private:
        using ResourceSetIterator = std::unordered_set<GPUResource*>::iterator;

        void CheckFrameValidity();

        uint64_t mFrameNumber = 0;
        const HAL::Device* mDevice = nullptr;
        SegregatedPoolsResourceAllocator* mResourceAllocator = nullptr;
        ResourceStateTracker* mStateTracker = nullptr;
        PoolDescriptorAllocator* mDescriptorAllocator = nullptr;
        CopyRequestManager* mCopyRequestManager = nullptr;
        std::unordered_set<GPUResource*> mAllocatedResources;
    };

}