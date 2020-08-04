#pragma once

#include "SegregatedPoolsResourceAllocator.hpp"
#include "ResourceStateTracker.hpp"
#include "PoolDescriptorAllocator.hpp"
#include "Buffer.hpp"
#include "Texture.hpp"

#include <unordered_set>

namespace Memory
{

    class GPUResourceProducer : public GPUResource::CopyCommandListProvider
    {
    public:
        using BufferPtr = std::unique_ptr<Buffer, std::function<void(Buffer*)>>;
        using TexturePtr = std::unique_ptr<Texture, std::function<void(Texture*)>>;

        GPUResourceProducer(
            const HAL::Device* device, 
            SegregatedPoolsResourceAllocator* resourceAllocator,
            ResourceStateTracker* stateTracker,
            PoolDescriptorAllocator* descriptorAllocator
        );

        template <class Element>
        BufferPtr NewBuffer(const HAL::BufferProperties<Element>& properties, GPUResource::UploadStrategy uploadStrategy = GPUResource::UploadStrategy::Automatic);
        TexturePtr NewTexture(const HAL::TextureProperties& properties);

        template <class Element>
        BufferPtr NewBuffer(const HAL::BufferProperties<Element>& properties, const HAL::Heap& explicitHeap, uint64_t heapOffset);
        TexturePtr NewTexture(const HAL::TextureProperties& properties, const HAL::Heap& explicitHeap, uint64_t heapOffset);

        TexturePtr NewTexture(HAL::Texture* existingTexture);
        
        void SetCommandList(HAL::CopyCommandListBase* commandList);

        void BeginFrame(uint64_t frameNumber);
        void EndFrame(uint64_t frameNumber);

        HAL::CopyCommandListBase* CommandList() override;

    private:
        using ResourceSetIterator = std::unordered_set<GPUResource*>::iterator;

        uint64_t mFrameNumber = 0;
        const HAL::Device* mDevice = nullptr;
        SegregatedPoolsResourceAllocator* mResourceAllocator = nullptr;
        ResourceStateTracker* mStateTracker = nullptr;
        PoolDescriptorAllocator* mDescriptorAllocator = nullptr;
        HAL::CopyCommandListBase* mCommandList = nullptr;
        std::unordered_set<GPUResource*> mAllocatedResources;
    };

}

#include "GPUResourceProducer.inl"