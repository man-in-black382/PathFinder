#include "TopRTAS.hpp"

namespace PathFinder
{


    TopRTAS::TopRTAS(const HAL::Device* device, Memory::GPUResourceProducer* resourceProducer)
        : RTAS(resourceProducer), mAccelerationStructure{ device } {}

    void TopRTAS::AddInstance(const BottomRTAS& blas, uint32_t instanceId, const glm::mat4& transform)
    {
        mAccelerationStructure.AddInstance(blas.HALAccelerationStructure(), instanceId, transform);
    }

    void TopRTAS::Build()
    {
        auto memoryRequirements = mAccelerationStructure.QueryMemoryRequirements();
        AllocateBuffersForBuildIfNeeded(memoryRequirements.DestinationBufferMaxSizeInBytes, memoryRequirements.BuildScratchBufferSizeInBytes);
        AllocateInstanceBufferIfNeeded(memoryRequirements.InstanceBufferSizeInBytes);

        mInstanceBuffer->RequestWrite();

        mAccelerationStructure.SetBuffers(
            mInstanceBuffer->WriteOnlyPtr(),
            mInstanceBuffer->HALBuffer(),
            mDestinationBuffer->HALBuffer(),
            mScratchBuffer->HALBuffer(),
            nullptr);
    }

    void TopRTAS::Update()
    {
        auto memoryRequirements = mAccelerationStructure.QueryMemoryRequirements();
        AllocateBuffersForUpdateIfNeeded(memoryRequirements.DestinationBufferMaxSizeInBytes, memoryRequirements.UpdateScratchBufferSizeInBytes);
        AllocateInstanceBufferIfNeeded(memoryRequirements.InstanceBufferSizeInBytes);

        mInstanceBuffer->RequestWrite();

        mAccelerationStructure.SetBuffers(
            mInstanceBuffer->WriteOnlyPtr(),
            mInstanceBuffer->HALBuffer(),
            mDestinationBuffer->HALBuffer(),
            mScratchBuffer->HALBuffer(),
            mUpdateSourceBuffer->HALBuffer());
    }

    void TopRTAS::Clear()
    {
        RTAS::Clear();
        mAccelerationStructure.Clear();
    }

    void TopRTAS::AllocateInstanceBufferIfNeeded(uint64_t bufferSize)
    {
        if (!mInstanceBuffer || mInstanceBuffer->Capacity() < bufferSize)
        {
            HAL::BufferProperties properties{ bufferSize };
            mInstanceBuffer = mResourceProducer->NewBuffer(properties, Memory::GPUResource::UploadStrategy::DirectAccess);
        }

        ApplyDebugName();
    }

    void TopRTAS::ApplyDebugName()
    {
        RTAS::ApplyDebugName();
        if (mInstanceBuffer) mInstanceBuffer->SetDebugName(mDebugName + " Instance Buffer");
    }

}
