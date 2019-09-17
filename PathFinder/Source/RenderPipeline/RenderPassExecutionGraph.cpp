#include "RenderPassExecutionGraph.hpp"

namespace PathFinder
{

    void RenderPassExecutionGraph::AddPass(const RenderPass* pass)
    {
        mExecutionOrder.push_back(pass);
    }

}
