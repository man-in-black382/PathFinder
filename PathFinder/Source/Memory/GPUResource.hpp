#pragma once

#include "SegregatedPoolsResourceAllocator.hpp"
#include "ResourceStateTracker.hpp"
#include "PoolDescriptorAllocator.hpp"
#include "CopyRequestManager.hpp"

#include <HardwareAbstractionLayer/Resource.hpp>
#include <HardwareAbstractionLayer/CommandList.hpp>

#include <queue>

namespace Memory
{

    class GPUResource
    {
    public:
        enum class AccessStrategy
        {
            DirectUpload,
            DirectReadback,
            Automatic
        };

        GPUResource(
            AccessStrategy accessStrategy,
            ResourceStateTracker* stateTracker,
            SegregatedPoolsResourceAllocator* resourceAllocator,
            PoolDescriptorAllocator* descriptorAllocator,
            CopyRequestManager* copyRequestManager);

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
        T* WriteOnlyPtr();

        template <class T = uint8_t>
        void Write(const T* data, uint64_t startIndex, uint64_t objectCount, uint64_t objectAlignment = 1);

        void RequestWrite();
        void RequestRead();
        void RequestNewState(HAL::ResourceState newState);
        void RequestNewSubresourceStates(const ResourceStateTracker::SubresourceStateList& newStates);
        void SetDebugName(const std::string& name);

        virtual void BeginFrame(uint64_t frameNumber);
        virtual void EndFrame(uint64_t frameNumber);
        virtual const HAL::Resource* HALResource() const;

    protected:
        using BufferFrameNumberPair = std::pair<SegregatedPoolsResourceAllocator::BufferPtr, uint64_t>;

        HAL::Buffer* CurrentFrameUploadBuffer();
        HAL::Buffer* CurrentFrameReadbackBuffer();
        const HAL::Buffer* CurrentFrameUploadBuffer() const;
        const HAL::Buffer* CurrentFrameReadbackBuffer() const;

        virtual void ApplyDebugName();
        virtual uint64_t ResourceSizeInBytes() const = 0;
        virtual CopyRequestManager::CopyCommand GetUploadCommands() = 0;
        virtual CopyRequestManager::CopyCommand GetReadbackCommands() = 0;

        AccessStrategy mAccessStrategy = AccessStrategy::Automatic;
        ResourceStateTracker* mStateTracker;
        SegregatedPoolsResourceAllocator* mResourceAllocator;
        PoolDescriptorAllocator* mDescriptorAllocator;
        CopyRequestManager* mCopyRequestManager;

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

