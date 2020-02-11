#pragma once

#include "SegregatedPoolsResourceAllocator.hpp"
#include "ResourceStateTracker.hpp"
#include "PoolDescriptorAllocator.hpp"

#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/CommandList.hpp"

#include <queue>

namespace Memory
{
   
    class GPUResource
    {
    public:
        enum class UploadStrategy
        {
            DirectAccess,
            Automatic
        };

        GPUResource(
            UploadStrategy uploadStrategy,
            ResourceStateTracker* stateTracker,
            SegregatedPoolsResourceAllocator* resourceAllocator,
            PoolDescriptorAllocator* descriptorAllocator,
            HAL::CopyCommandListBase* commandList);

        GPUResource(const GPUResource& that) = delete;
        GPUResource(GPUResource&& that) = default;
        GPUResource& operator=(const GPUResource& that) = delete;
        GPUResource& operator=(GPUResource&& that) = default;

        virtual ~GPUResource() = 0;

        template <class T>
        using ReadbackSession = std::function<void(const T*)>;

        template <class T = uint8_t>
        void Read(const ReadbackSession<T>& session) const;

        template <class T = uint8_t>
        T * Write();

        template <class T = uint8_t>
        void Write(const T* data, uint64_t startIndex, uint64_t objectCount, uint64_t objectAlignment = 1);

        virtual void RequestWrite();
        virtual void RequestRead();

        void RequestNewState(HAL::ResourceState newState);

        void BeginFrame(uint64_t frameNumber);
        void EndFrame(uint64_t frameNumber);

        void SetCommandList(HAL::CopyCommandListBase* commandList);
        void SetDebugName(const std::string& name);

        virtual const HAL::Resource* HALResource() const;

    protected:
        inline HAL::CopyCommandListBase* CommandList() { return mCommandList; }
        inline ResourceStateTracker* StateTracker() { return mStateTracker; }
        inline PoolDescriptorAllocator* DescriptorAllocator() { return mDescriptorAllocator; }
        inline UploadStrategy CurrentUploadStrategy() { return mUploadStrategy; }
        
        HAL::Buffer* CurrentFrameUploadBuffer();
        HAL::Buffer* CurrentFrameReadbackBuffer();
        const HAL::Buffer* CurrentFrameUploadBuffer() const;
        const HAL::Buffer* CurrentFrameReadbackBuffer() const;

    private:
        using BufferFrameNumberPair = std::pair<SegregatedPoolsResourceAllocator::BufferPtr, uint64_t>;

        uint64_t mFrameNumber = 0;
        UploadStrategy mUploadStrategy = UploadStrategy::Automatic;

        ResourceStateTracker* mStateTracker;
        SegregatedPoolsResourceAllocator* mResourceAllocator;
        PoolDescriptorAllocator* mDescriptorAllocator;
        HAL::CopyCommandListBase* mCommandList;

        SegregatedPoolsResourceAllocator::BufferPtr mCompletedReadbackBuffer;

        std::queue<BufferFrameNumberPair> mUploadBuffers;
        std::queue<BufferFrameNumberPair> mReadbackBuffers;
    };

}

#include "GPUResource.inl"

