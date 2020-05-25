#pragma once

#include "RenderPassExecutionGraph.hpp"

#include <functional>

namespace PathFinder
{

    class PipelineResourceSchedulingRequest
    {
    public:
        using Action = std::function<void()>;

        PipelineResourceSchedulingRequest(Foundation::Name resourceName, const RenderPassExecutionGraph::Node& renderPassGraphNode, const Action& requestedAction);

    private:
        RenderPassExecutionGraph::Node mRenderPassGraphNode;
        Foundation::Name mResourceName;
        Action mRequestedAction;

    public:
        inline const auto& RenderPassGraphNode() const { return mRenderPassGraphNode; }
        inline auto ResourceName() const { return mResourceName; }
        inline const auto& RequestedAction() const { return mRequestedAction; }
    };

}
