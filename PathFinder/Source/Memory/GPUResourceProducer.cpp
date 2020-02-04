#include "GPUResourceProducer.hpp"

namespace Memory
{

    GPUResourceProducer::GPUResourceProducer(SegregatedPoolsResourceAllocator* resourceAllocator, HAL::CopyCommandListBase* commandList)
        : mAllocator{ resourceAllocator }, mCommandList{ commandList }
    {

    }

    GPUResourceProducer::TexturePtr GPUResourceProducer::NewTexture(const HAL::Texture::Properties& properties)
    {
        Texture* texture = new Texture{ properties, mAllocator, mCommandList };
        auto [iter, success] = mAllocatedResources.insert(texture);

        auto deallocationCallback = [this, iter](Texture* texture)
        {
            mAllocatedResources.erase(iter);
            delete texture;
        };

        return TexturePtr{ texture, deallocationCallback };
    }

    void GPUResourceProducer::SetCommandList(HAL::CopyCommandListBase* commandList)
    {
        mCommandList = commandList;
    }

    void GPUResourceProducer::BeginFrame(uint64_t frameNumber)
    {
        for (GPUResource* resource : mAllocatedResources)
        {
            resource->SetCommandList(mCommandList);
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
