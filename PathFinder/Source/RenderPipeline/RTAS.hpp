#pragma once

#include <HardwareAbstractionLayer/RayTracingAccelerationStructure.hpp>
#include <HardwareAbstractionLayer/ResourceBarrier.hpp>

#include <Memory/GPUResourceProducer.hpp>

namespace PathFinder
{

    class RTAS
    {
    public:
        RTAS(Memory::GPUResourceProducer* resourceProducer);
        RTAS(RTAS&& that) = default;
        RTAS(const RTAS& that) = delete;
        RTAS& operator=(RTAS&& that) = default;
        RTAS& operator=(const RTAS& that) = delete;
        virtual ~RTAS() = 0;

        void AllocateBuffersForBuildIfNeeded(uint64_t destinationBufferSize, uint64_t scratchBufferSize);
        void AllocateBuffersForUpdateIfNeeded(uint64_t destinationBufferSize, uint64_t scratchBufferSize);
        virtual void Clear();

        void SetDebugName(const std::string& name);

    protected:
        virtual void ApplyDebugName();

        Memory::GPUResourceProducer* mResourceProducer;

        // Scratch buffer used for both builds and updates
        Memory::GPUResourceProducer::BufferPtr mScratchBuffer;

        // A previously built BVH that is used as a source for update (optional)
        Memory::GPUResourceProducer::BufferPtr mUpdateSourceBuffer;

        // A resulting BVH for build or update
        Memory::GPUResourceProducer::BufferPtr mDestinationBuffer;

        HAL::UnorderedAccessResourceBarrier mUABarrier;
        
        std::string mDebugName;        

    public:
        inline const auto& UABarrier() const { return mUABarrier; }
        inline const auto AccelerationStructureBuffer() const { return mDestinationBuffer.get(); }
    };

}
