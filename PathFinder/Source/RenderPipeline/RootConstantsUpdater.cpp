#include "RootConstantsUpdater.hpp"

namespace PathFinder
{

    RootConstantsUpdater::RootConstantsUpdater(PipelineResourceStorage* storage, const RenderPassGraph::Node* passNode)
        : mResourceStorage{ storage }, mPassNode{ passNode } {}

}
