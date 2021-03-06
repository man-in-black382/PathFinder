#pragma once

namespace PathFinder
{

    struct PipelineSettings
    {
        bool IsMemoryAliasingEnabled = true;
        bool IsAsyncComputeEnabled = true;
        bool IsSplitBarriersEnabled = true;
    };

}
