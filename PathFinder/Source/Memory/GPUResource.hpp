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

        class CopyCommandListProvider
        {
        public:
            virtual HAL::CopyCommandListBase* CommandList() = 0;
        };

        GPUResource(
            UploadStrategy uploadStrategy,
            ResourceStateTracker* stateTracker,
            SegregatedPoolsResourceAllocator* resourceAllocator,
            PoolDescriptorAllocator* descriptorAllocator,
            CopyCommandListProvider* commandListProvider);

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
        T * WriteOnlyPtr();

        template <class T = uint8_t>
        void Write(const T* data, uint64_t startIndex, uint64_t objectCount, uint64_t objectAlignment = 1);

        void RequestWrite(HAL::CopyCommandListBase* customCmdList = nullptr);
        void RequestRead(HAL::CopyCommandListBase* customCmdList = nullptr);
        void RequestNewState(HAL::ResourceState newState);
        void RequestNewSubresourceStates(const ResourceStateTracker::SubresourceStateList& newStates);
        void BeginFrame(uint64_t frameNumber);
        void EndFrame(uint64_t frameNumber);
        void SetDebugName(const std::string& name);

        virtual const HAL::Resource* HALResource() const;

    protected:
        using BufferFrameNumberPair = std::pair<SegregatedPoolsResourceAllocator::BufferPtr, uint64_t>;

        HAL::Buffer* CurrentFrameUploadBuffer();
        HAL::Buffer* CurrentFrameReadbackBuffer();
        const HAL::Buffer* CurrentFrameUploadBuffer() const;
        const HAL::Buffer* CurrentFrameReadbackBuffer() const;

        virtual void ApplyDebugName();
        virtual uint64_t ResourceSizeInBytes() const = 0;
        virtual void RecordUploadCommands() = 0;
        virtual void RecordReadbackCommands() = 0;

        UploadStrategy mUploadStrategy = UploadStrategy::Automatic;
        ResourceStateTracker* mStateTracker;
        SegregatedPoolsResourceAllocator* mResourceAllocator;
        PoolDescriptorAllocator* mDescriptorAllocator;
        CopyCommandListProvider* mCommandListProvider;

        std::queue<BufferFrameNumberPair> mUploadBuffers;
        std::queue<BufferFrameNumberPair> mReadbackBuffers;

        std::string mDebugName;
        uint64_t mFrameNumber = 0;

    private:
        void AllocateNewUploadBuffer();
        void AllocateNewReadbackBuffer();

        SegregatedPoolsResourceAllocator::BufferPtr mCompletedReadbackBuffer;
        SegregatedPoolsResourceAllocator::BufferPtr mCompletedUploadBuffer;
    };

}

#include "GPUResource.inl"

