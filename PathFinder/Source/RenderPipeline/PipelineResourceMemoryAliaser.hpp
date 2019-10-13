#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"

#include "PipelineResourceAllocation.hpp"
#include "RenderPassExecutionGraph.hpp"

namespace PathFinder
{

    class PipelineResourceMemoryAliaser
    {
    public:
        PipelineResourceMemoryAliaser(const RenderPassExecutionGraph* renderPassGraph);

        void AddAllocation(PipelineResourceAllocation* allocation);
        //void O

    private:
        struct Timeline
        {
            uint16_t Start;
            uint16_t End;
        };

        struct AliasingMetadata
        {
            Timeline ResourceTimeline;
            uint64_t ResourceSize;
        };

        bool TimelinesIntersect(const Timeline& first, const Timeline& second);
    };

}
