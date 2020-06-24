#include "RTAS.hpp"

namespace PathFinder
{

    RTAS::RTAS(Memory::GPUResourceProducer* resourceProducer)
        : mResourceProducer{ resourceProducer }, mUABarrier{ nullptr } {}

    RTAS::~RTAS() {}

    void RTAS::AllocateBuffersForBuildIfNeeded(uint64_t destinationBufferSize, uint64_t scratchBufferSize)
    {
        if (!mDestinationBuffer || mDestinationBuffer->Capacity() < destinationBufferSize)
        {
            HAL::Buffer::Properties properties{ destinationBufferSize, 1, HAL::ResourceState::RaytracingAccelerationStructure, HAL::ResourceState::UnorderedAccess };
            mDestinationBuffer = mResourceProducer->NewBuffer(properties);
            mUABarrier = HAL::UnorderedAccessResourceBarrier{ mDestinationBuffer->HALBuffer() };
        }

        if (!mScratchBuffer || mScratchBuffer->Capacity() < scratchBufferSize)
        {
            HAL::Buffer::Properties properties{ scratchBufferSize, 1, HAL::ResourceState::UnorderedAccess };
            mScratchBuffer = mResourceProducer->NewBuffer(properties);
        }

        mUpdateSourceBuffer = nullptr;

        ApplyDebugName();
    }

    void RTAS::AllocateBuffersForUpdateIfNeeded(uint64_t destinationBufferSize, uint64_t scratchBufferSize)
    {
        assert_format(mDestinationBuffer, "Cannot update an acceleration structure that wasn't built at least once yet");
        
        // Use last destination buffer as a source of update
        mUpdateSourceBuffer = std::move(mDestinationBuffer);

        if (!mDestinationBuffer || mDestinationBuffer->Capacity() < destinationBufferSize)
        {
            HAL::Buffer::Properties properties{ destinationBufferSize, 1, HAL::ResourceState::RaytracingAccelerationStructure, HAL::ResourceState::UnorderedAccess };
            mDestinationBuffer = mResourceProducer->NewBuffer(properties);
            mUABarrier = HAL::UnorderedAccessResourceBarrier{ mDestinationBuffer->HALBuffer() };
        }

        if (!mScratchBuffer || mScratchBuffer->Capacity() < scratchBufferSize)
        {
            HAL::Buffer::Properties properties{ scratchBufferSize, 1, HAL::ResourceState::UnorderedAccess };
            mScratchBuffer = mResourceProducer->NewBuffer(properties);
        }

        ApplyDebugName();
    }

    void RTAS::Clear()
    {
    }

    void RTAS::SetDebugName(const std::string& name)
    {
        mDebugName = name;
        ApplyDebugName();
    }

    void RTAS::ApplyDebugName()
    {
        if (mDestinationBuffer) mDestinationBuffer->SetDebugName(mDebugName + " Destination Buffer");
        if (mScratchBuffer) mScratchBuffer->SetDebugName(mDebugName + " Scratch Buffer");
        if (mUpdateSourceBuffer) mUpdateSourceBuffer->SetDebugName(mDebugName + " Update Source Buffer");
    }

}
