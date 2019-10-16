#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"

#include "PipelineResourceAllocation.hpp"
#include "RenderPassExecutionGraph.hpp"

#include <set>

namespace PathFinder
{

    class PipelineResourceStateOptimizer
    {
    public:
        PipelineResourceStateOptimizer(const RenderPassExecutionGraph* renderPassGraph);

        void AddAllocation(PipelineResourceAllocation* allocation);
        void Optimize();

    private:
        void CollapseStateSequences(const PipelineResourceAllocation* allocation);

        // To be used between function calls to avoid memory reallocations
        std::vector<std::pair<Foundation::Name, HAL::ResourceState>> mCollapsedStateSequences;

        std::vector<PipelineResourceAllocation*> mAllocations;
        const RenderPassExecutionGraph* mRenderPassGraph;
    };

}
