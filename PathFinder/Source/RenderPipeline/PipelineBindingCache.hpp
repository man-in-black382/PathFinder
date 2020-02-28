#pragma once

#include "../HardwareAbstractionLayer/Buffer.hpp"
#include "../HardwareAbstractionLayer/PipelineState.hpp"

namespace PathFinder
{

    class PipelineBindingCache
    {
    public:
        struct StateBindingNecessity
        {

        };

    private:
        const HAL::GraphicsPipelineState* mAppliedGraphicsState = nullptr;
        const HAL::ComputePipelineState* mAppliedComputeState = nullptr;
        const HAL::RayTracingPipelineState* mAppliedRayTracingState = nullptr;
        const HAL::RootSignature* mAppliedGraphicsRootSignature = nullptr;
        const HAL::RootSignature* mAppliedComputeRootSignature = nullptr;
        const HAL::Buffer* mBoundGlobalConstantBufferCompute = nullptr;
        const HAL::Buffer* mBoundFrameConstantBufferCompute = nullptr;
        const HAL::Buffer* mBoundPassConstantBufferCompute = nullptr;
        const HAL::Buffer* mBoundPassDebugBufferCompute = nullptr;
        const HAL::Buffer* mBoundGlobalConstantBufferGraphics = nullptr;
        const HAL::Buffer* mBoundFrameConstantBufferGraphics = nullptr;
        const HAL::Buffer* mBoundPassConstantBufferGraphics = nullptr;
        const HAL::Buffer* mBoundPassDebugBufferGraphics = nullptr;
    };

}
