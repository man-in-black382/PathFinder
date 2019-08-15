#pragma once

#include "../HardwareAbstractionLayer/PipelineState.hpp"
#include "VertexLayouts.hpp"

namespace PathFinder
{

    class IPipelineStateManager
    {
    public:
        using PSOName = Foundation::Name;
        using RootSignatureName = Foundation::Name;

        virtual HAL::GraphicsPipelineState CloneDefaultGraphicsState() = 0;
        virtual HAL::GraphicsPipelineState CloneExistingGraphicsState(PSOName name) = 0;
        virtual HAL::ComputePipelineState CloneDefaultComputeState() = 0;
        virtual HAL::ComputePipelineState CloneExistingComputeState(PSOName name) = 0;
        virtual HAL::RootSignature CloneBaseRootSignature() = 0;

        virtual void StoreRootSignature(RootSignatureName name, const HAL::RootSignature& signature) = 0;
        virtual void StoreGraphicsState(PSOName name, const HAL::GraphicsPipelineState& pso, RootSignatureName assosiatedSignatureName) = 0;
        virtual void StoreComputeState(PSOName name, const HAL::ComputePipelineState& pso, RootSignatureName assosiatedSignatureName) = 0;
    };

}
