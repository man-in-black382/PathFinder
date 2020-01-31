#include "GPUResourceProducer.hpp"

namespace Memory
{

    GPUResourceProducer::GPUResourceProducer(SegregatedPoolsResourceAllocator* resourceAllocator, HAL::CopyCommandListBase* commandList)
        : mAllocator{ resourceAllocator }, mCommandList{ commandList }
    {

    }

    std::unique_ptr<Texture> GPUResourceProducer::NewTexture(const HAL::Texture::Properties& properties)
    {
        Texture* texture = new Texture{ properties, mAllocator, mCommandList };
        auto [iter, success] = mAllocatedResources.insert(texture);

        auto deallocationCallback = [this, iter](Texture* texture)
        {
            mAllocatedResources.erase(iter);
            delete texture;
        };

        auto ptr = std::unique_ptr<Texture, decltype(deallocationCallback)>(texture, deallocationCallback);

        return 
    }

    void GPUResourceProducer::SetCommandList(HAL::CopyCommandListBase* commandList)
    {

    }

    void GPUResourceProducer::BeginFrame(uint64_t frameNumber)
    {

    }

    void GPUResourceProducer::EndFrame(uint64_t frameNumber)
    {

    }

}
