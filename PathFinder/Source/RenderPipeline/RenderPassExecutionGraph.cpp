#include "RenderPassExecutionGraph.hpp"

#include "RenderPass.hpp"

namespace PathFinder
{

    void RenderPassExecutionGraph::AddPass(RenderPass* pass)
    {
        switch (pass->PurposeInPipeline())
        {
        case RenderPass::Purpose::Default:
            mDefaultPasses.push_back(pass); 
            break;

        case RenderPass::Purpose::Setup:
            mSetupPasses.push_front(pass);
            break;

        case RenderPass::Purpose::AssetProcessing:
            mAssetProcessingPasses.push_back(pass);
            break;
        }     

        mAllPasses.push_back(pass);
    }

    uint32_t RenderPassExecutionGraph::IndexOfPass(const RenderPass* pass) const
    {
        auto it = std::find(mDefaultPasses.cbegin(), mDefaultPasses.cend(), pass);
        return std::distance(mDefaultPasses.cbegin(), it);
    }

    uint32_t RenderPassExecutionGraph::IndexOfPass(Foundation::Name passName) const
    {
        auto it = std::find_if(mDefaultPasses.cbegin(), mDefaultPasses.cend(), [&passName](auto pass) { return pass->Name() == passName; });
        return std::distance(mDefaultPasses.cbegin(), it);
    }

}
