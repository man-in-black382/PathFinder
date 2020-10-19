#pragma once

#include "RTAS.hpp"
#include "BottomRTAS.hpp"

namespace PathFinder
{

    class TopRTAS : public RTAS
    {
    public:
        TopRTAS(const HAL::Device* device, Memory::GPUResourceProducer* resourceProducer);
        TopRTAS(TopRTAS&& that) = default;
        TopRTAS(const TopRTAS& that) = delete;
        TopRTAS& operator=(TopRTAS&& that) = default;
        TopRTAS& operator=(const TopRTAS& that) = delete;
        ~TopRTAS() = default;

        void AddInstance(const BottomRTAS& blas, const HAL::RayTracingTopAccelerationStructure::InstanceInfo& instanceInfo, const glm::mat4& transform);

        void Build();
        void Update();
        void Clear() override;

    protected:
        void ApplyDebugName() override;

    private:
        void AllocateInstanceBufferIfNeeded(uint64_t bufferSize);

        Memory::GPUResourceProducer::BufferPtr mInstanceBuffer;
        HAL::RayTracingTopAccelerationStructure mAccelerationStructure;

    public:
        inline const auto& HALAccelerationStructure() const { return mAccelerationStructure; }
    };

}
