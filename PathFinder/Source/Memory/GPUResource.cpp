#include "GPUResource.hpp"

namespace Memory
{

    GPUResource::GPUResource(
        UploadStrategy uploadStrategy, 
        std::unique_ptr<HAL::Resource> resource,
        SegregatedPoolsResourceAllocator* resourceAllocator, 
        HAL::CopyCommandListBase* commandList)
        : 
        mUploadStrategy{ uploadStrategy }, 
        mResource{ std::move(resource) }, 
        mAllocator{ resourceAllocator }, 
        mCommandList{ commandList } 
    {
        
    }

    GPUResource::~GPUResource() {}

    void GPUResource::RequestWrite()
    {
        // Upload is already requested in current frame
        if (mUploadBuffers.back().second == mFrameNumber)
        {
            return;
        }

        HAL::Buffer::Properties properties{ mResource->TotalMemory() };
        mUploadBuffers.emplace(mAllocator->AllocateBuffer<uint8_t>(properties, HAL::CPUAccessibleHeapType::Upload), mFrameNumber);
    }

    void GPUResource::RequestRead()
    {
        assert_format(mUploadStrategy != UploadStrategy::DirectAccess, "DirectAccess upload resource does not support reads");

        // Readback is already requested in current frame
        if (mReadbackBuffers.back().second == mFrameNumber)
        {
            return;
        }

        HAL::Buffer::Properties properties{ mResource->TotalMemory() };
        mReadbackBuffers.emplace(mAllocator->AllocateBuffer<uint8_t>(properties, HAL::CPUAccessibleHeapType::Readback), mFrameNumber);
    }

    void GPUResource::BeginFrame(uint64_t frameNumber)
    {
        mFrameNumber = frameNumber;
    }

    void GPUResource::EndFrame(uint64_t frameNumber)
    {
        // Get freshest completed upload buffer. Discard the rest.
        while (!mUploadBuffers.empty() && mUploadBuffers.front().second <= frameNumber)
        {
            mCompletedUploadBuffer = std::move(mUploadBuffers.front().first);
            mUploadBuffers.pop();
        }

        // Get freshest completed readback buffer. Discard the rest.
        while (!mReadbackBuffers.empty() && mReadbackBuffers.front().second <= frameNumber)
        {
            mCompletedReadbackBuffer = std::move(mReadbackBuffers.front().first);
            mReadbackBuffers.pop();
        }
    }

    void GPUResource::SetCommandList(HAL::CopyCommandListBase* commandList)
    {
        mCommandList = commandList;
    }

    const HAL::Resource* GPUResource::HALResource() const
    {
        return mUploadStrategy == UploadStrategy::Automatic ? mResource.get() : mCompletedUploadBuffer.get();
    }

    HAL::Buffer* GPUResource::CurrentFrameUploadBuffer()
    {
        return !mUploadBuffers.empty() && mUploadBuffers.back().second == mFrameNumber ? 
            mUploadBuffers.back().first.get() : nullptr;
    }

    HAL::Buffer* GPUResource::CurrentFrameReadbackBuffer()
    {
        return !mReadbackBuffers.empty() && mReadbackBuffers.back().second == mFrameNumber ?
            mReadbackBuffers.back().first.get() : nullptr;
    }

}
