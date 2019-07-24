#pragma once

#include "../HardwareAbstractionLayer/PipelineState.hpp"

namespace PathFinder
{

    class IPipelineStateManager
    {
    public:
        using PSOName = Foundation::Name;

        virtual GraphicsPipelineState CloneDefaultGraphicsState() = 0;
        virtual GraphicsPipelineState CloneExistingGraphicsState(PSOName name) = 0;
        virtual void StoreGraphicsState(PSOName name, const GraphicsPipelineState& pso) = 0;
    };

}
