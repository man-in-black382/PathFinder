#include "RenderPassExecutionGraph.hpp"

#include "RenderPass.hpp"

namespace PathFinder
{

    void RenderPassExecutionGraph::AddPass(RenderPass* pass)
    {
        mExecutionOrder.push_back(pass);
    }

    uint32_t RenderPassExecutionGraph::IndexOfPass(const RenderPass* pass) const
    {
        auto it = std::find(mExecutionOrder.cbegin(), mExecutionOrder.cend(), pass);
        return std::distance(mExecutionOrder.cbegin(), it);
    }

    uint32_t RenderPassExecutionGraph::IndexOfPass(Foundation::Name passName) const
    {
        auto it = std::find_if(mExecutionOrder.cbegin(), mExecutionOrder.cend(), [&passName](auto pass) { return pass->Name() == passName; });
        return std::distance(mExecutionOrder.cbegin(), it);
    }

}
