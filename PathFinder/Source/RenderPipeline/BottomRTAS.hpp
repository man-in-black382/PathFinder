#pragma once

#include "RTAS.hpp"

namespace PathFinder
{

    class BottomRTAS : public RTAS
    {
    public:
        BottomRTAS(const HAL::Device* device, Memory::GPUResourceProducer* resourceProducer);
        BottomRTAS(BottomRTAS&& that) = default;
        BottomRTAS(const BottomRTAS& that) = delete;
        BottomRTAS& operator=(BottomRTAS&& that) = default;
        BottomRTAS& operator=(const BottomRTAS& that) = delete;
        ~BottomRTAS() = default;

        void AddGeometry(const HAL::RayTracingGeometry& geometry);
        void Build();
        void Update();
        void Clear() override;

    private:
        HAL::RayTracingBottomAccelerationStructure mAccelerationStructure;

    public:
        inline const auto& HALAccelerationStructure() const { return mAccelerationStructure; }
    };

}
