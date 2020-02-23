#include "RenderPassExecutionGraph.hpp"

#include "RenderPass.hpp"

namespace PathFinder
{

    void RenderPassExecutionGraph::AddPass(const RenderPassMetadata& passMetadata)
    {
        switch (passMetadata.Purpose)
        {
        case RenderPassPurpose::Default:
            mDefaultPasses.emplace_back(passMetadata, mDefaultPasses.size());
            break;

        case RenderPassPurpose::Setup:
            mSetupPasses.emplace_back(passMetadata, mSetupPasses.size());
            break;

        case RenderPassPurpose::AssetProcessing:
            mAssetProcessingPasses.emplace_back(passMetadata, mAssetProcessingPasses.size());
            break;
        }

        mAllPasses.emplace_back(passMetadata, mAllPasses.size());
    }

}
