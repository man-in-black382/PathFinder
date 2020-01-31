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
        GPUResourceProducer(SegregatedPoolsResourceAllocator* resourceAllocator, HAL::CopyCommandListBase* commandList);

        template <class Element>
        std::unique_ptr<Buffer> NewBuffer(const HAL::Buffer::Properties<Element>& properties);
        std::unique_ptr<Texture> NewTexture(const HAL::Texture::Properties& properties);
        
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

#include "GPUResourceProducer.hpp"