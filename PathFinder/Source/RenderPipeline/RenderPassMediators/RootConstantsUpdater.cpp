#include "RootConstantsUpdater.hpp"

namespace PathFinder
{

    RootConstantsUpdater::RootConstantsUpdater(PipelineResourceStorage* storage, const RenderPassGraph* passGraph, uint64_t graphNodeIndex)
        : mResourceStorage{ storage }, mPassGraph{ passGraph }, mGraphNodeIndex{ graphNodeIndex } {}

}
