#pragma once

#include "../HardwareAbstractionLayer/PipelineState.hpp"
#include "CommonInputAssemblerLayouts.hpp"

namespace PathFinder
{

    class IPipelineStateManager
    {
    public:
        using PSOName = Foundation::Name;

        virtual HAL::GraphicsPipelineState CloneDefaultGraphicsState() = 0;
        virtual HAL::GraphicsPipelineState CloneExistingGraphicsState(PSOName name) = 0;
        virtual void StoreGraphicsState(PSOName name, const HAL::GraphicsPipelineState& pso) = 0;
    };

}
