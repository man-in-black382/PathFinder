#pragma once

#include "PipelineNames.hpp"

namespace PathFinder
{

    // Here we define helpers that will resolve resource names for situations when certain render passes are disabled
    // and resource read/write chain needs to be reconfigured. We should do it here to avoid performing checks all over the engine.

    Foundation::Name SMAAEdgeDetectionPassInputSRName(bool isGIDebugEnabled);

}
