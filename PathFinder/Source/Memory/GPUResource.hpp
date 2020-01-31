#pragma once

#include "SegregatedPoolsResourceAllocator.hpp"

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

        void BeginFrame(uint64_t frameNumber);
        void EndFrame(uint64_t frameNumber);

        void SetCommandList(HAL::CopyCommandListBase* commandList);

        const HAL::Resource* HALResource() const;

    protected:
        GPUResource(
            UploadStrategy uploadStrategy,
            std::unique_ptr<HAL::Resource> resource,
            SegregatedPoolsResourceAllocator* resourceAllocator,
            HAL::CopyCommandListBase* commandList
        );

        inline HAL::CopyCommandListBase* CommandList() { return mCommandList; }
        
        HAL::Buffer* CurrentFrameUploadBuffer();
        HAL::Buffer* CurrentFrameReadbackBuffer();

    private:
        using BufferFrameNumberPair = std::pair<std::unique_ptr<HAL::Buffer>, uint64_t>;

        uint64_t mFrameNumber = 0;

        UploadStrategy mUploadStrategy = UploadStrategy::Automatic;
        SegregatedPoolsResourceAllocator* mAllocator;
        HAL::CopyCommandListBase* mCommandList;

        std::unique_ptr<HAL::Buffer> mCompletedReadbackBuffer;
        std::unique_ptr<HAL::Buffer> mCompletedUploadBuffer;

        std::queue<BufferFrameNumberPair> mUploadBuffers;
        std::queue<BufferFrameNumberPair> mReadbackBuffers;

        std::unique_ptr<HAL::Resource> mResource;
    };

}

#include "GPUResource.inl"

