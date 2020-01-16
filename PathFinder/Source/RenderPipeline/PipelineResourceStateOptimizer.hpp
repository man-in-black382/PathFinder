#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"

#include "PipelineResourceSchedulingInfo.hpp"
#include "RenderPassExecutionGraph.hpp"

#include <set>

namespace PathFinder
{

    class PipelineResourceStateOptimizer
    {
    public:
        PipelineResourceStateOptimizer(const RenderPassExecutionGraph* renderPassGraph);

        void AddAllocation(PipelineResourceSchedulingInfo* allocation);
        void Optimize();

    private:
        void CollapseStateSequences(PipelineResourceSchedulingInfo* allocation);

        // To be used between function calls to avoid memory reallocations
        std::vector<std::pair<Foundation::Name, HAL::ResourceState>> mCollapsedStateSequences;

        std::vector<PipelineResourceSchedulingInfo*> mAllocations;
        const RenderPassExecutionGraph* mRenderPassGraph;
    };

}
