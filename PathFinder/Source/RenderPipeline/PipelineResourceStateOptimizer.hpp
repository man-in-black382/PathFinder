#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"

#include "PipelineResourceSchedulingInfo.hpp"
#include "RenderPassExecutionGraph.hpp"

#include <set>

namespace PathFinder
{
    /// Preprocesses states by combining read-only state sequences
    /// and determining the need for UAV barriers
    class PipelineResourceStateOptimizer
    {
    public:
        PipelineResourceStateOptimizer(const RenderPassExecutionGraph* renderPassGraph);

        void AddSchedulingInfo(PipelineResourceSchedulingInfo* schedulingInfo);
        void Optimize();

    private:
        void CombineStateSequences(PipelineResourceSchedulingInfo* allocation, uint64_t resourceIndex);

        std::vector<std::pair<Foundation::Name, HAL::ResourceState>> mCombinedStateSequences;

        std::vector<PipelineResourceSchedulingInfo*> mSchedulingInfos;
        const RenderPassExecutionGraph* mRenderPassGraph;
    };

}
