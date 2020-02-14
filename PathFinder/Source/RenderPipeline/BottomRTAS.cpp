#include "BottomRTAS.hpp"

namespace PathFinder
{

    BottomRTAS::BottomRTAS(const HAL::Device* device, Memory::GPUResourceProducer* resourceProducer)
        : RTAS(resourceProducer), mAccelerationStructure{ device } {}

    void BottomRTAS::AddGeometry(const HAL::RayTracingGeometry& geometry)
    {
        mAccelerationStructure.AddGeometry(geometry);
    }

    void BottomRTAS::Build()
    {
        auto memoryRequirements = mAccelerationStructure.QueryMemoryRequirements();
        AllocateBuffersForBuildIfNeeded(memoryRequirements.DestinationBufferMaxSizeInBytes, memoryRequirements.BuildScratchBufferSizeInBytes);
        mAccelerationStructure.SetBuffers(mDestinationBuffer->HALBuffer(), mScratchBuffer->HALBuffer(), nullptr);
    }

    void BottomRTAS::Update()
    {
        auto memoryRequirements = mAccelerationStructure.QueryMemoryRequirements();
        AllocateBuffersForUpdateIfNeeded(memoryRequirements.DestinationBufferMaxSizeInBytes, memoryRequirements.UpdateScratchBufferSizeInBytes);
        mAccelerationStructure.SetBuffers(mDestinationBuffer->HALBuffer(), mScratchBuffer->HALBuffer(), mUpdateSourceBuffer->HALBuffer());
    }

    void BottomRTAS::Clear()
    {
        RTAS::Clear();
        mAccelerationStructure.Clear();
    }

}
