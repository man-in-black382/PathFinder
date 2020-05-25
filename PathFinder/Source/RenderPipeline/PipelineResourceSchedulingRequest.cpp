#include "PipelineResourceSchedulingRequest.hpp"

namespace PathFinder
{

    PipelineResourceSchedulingRequest::PipelineResourceSchedulingRequest(Foundation::Name resourceName, const RenderPassExecutionGraph::Node& renderPassGraphNode, const Action& requestedAction)
        : mResourceName{ resourceName }, mRenderPassGraphNode{ renderPassGraphNode }, mRequestedAction{ requestedAction } {}

}
