#pragma once

#include "SegregatedPoolsResourceAllocator.hpp"
#include "ResourceStateTracker.hpp"

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
            HAL::CopyCommandListBase* commandList
        );

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

        virtual void RequestWrite();
        virtual void RequestRead();

        void RequestNewState(HAL::ResourceState newState);

        void BeginFrame(uint64_t frameNumber);
        void EndFrame(uint64_t frameNumber);

        void SetCommandList(HAL::CopyCommandListBase* commandList);

        virtual const HAL::Resource* HALResource() const;

    protected:
        inline HAL::CopyCommandListBase* CommandList() { return mCommandList; }
        
        const HAL::Buffer* CurrentFrameUploadBuffer() const;
        const HAL::Buffer* CurrentFrameReadbackBuffer() const;

        UploadStrategy mUploadStrategy = UploadStrategy::Automatic;

    private:
        using BufferFrameNumberPair = std::pair<SegregatedPoolsResourceAllocator::BufferPtr, uint64_t>;

        uint64_t mFrameNumber = 0;

        ResourceStateTracker* mStateTracker;
        SegregatedPoolsResourceAllocator* mAllocator;
        HAL::CopyCommandListBase* mCommandList;

        SegregatedPoolsResourceAllocator::BufferPtr mCompletedReadbackBuffer;
        SegregatedPoolsResourceAllocator::BufferPtr mCompletedUploadBuffer;

        std::queue<BufferFrameNumberPair> mUploadBuffers;
        std::queue<BufferFrameNumberPair> mReadbackBuffers;
    };

}

#include "GPUResource.inl"

