#pragma once

#include "SegregatedPoolsResourceAllocator.hpp"
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

        GPUResourceProducer(SegregatedPoolsResourceAllocator* resourceAllocator, HAL::CopyCommandListBase* commandList);

        template <class Element>
        BufferPtr NewBuffer(const HAL::Buffer::Properties<Element>& properties, GPUResource::UploadStrategy uploadStrategy = GPUResource::UploadStrategy::Automatic);
        TexturePtr NewTexture(const HAL::Texture::Properties& properties);
        
        void SetCommandList(HAL::CopyCommandListBase* commandList);

        void BeginFrame(uint64_t frameNumber);
        void EndFrame(uint64_t frameNumber);

    private:
        using ResourceSetIterator = std::unordered_set<GPUResource*>::iterator;

        SegregatedPoolsResourceAllocator* mAllocator = nullptr;
        HAL::CopyCommandListBase* mCommandList = nullptr;
        std::unordered_set<GPUResource*> mAllocatedResources;
    };

}

#include "GPUResourceProducer.inl"