#pragma once

#include "../Foundation/Name.hpp"
#include "RenderPassMetadata.hpp"

#include <vector>

namespace PathFinder
{
    
    class RenderPassExecutionGraph
    {
    public:
        struct Node
        {
            RenderPassMetadata PassMetadata;
            uint64_t ExecutionIndex = 0;
        };

        void AddPass(const RenderPassMetadata& passMetadata);

    private:
        using RenderPassList = std::vector<Node>;

        RenderPassList mDefaultPasses;
        RenderPassList mSetupPasses;
        RenderPassList mAssetProcessingPasses;
        RenderPassList mAllPasses;

    public:
        inline const auto& DefaultPasses() const { return mDefaultPasses; }
        inline const auto& SetupPasses() const { return mSetupPasses; }
        inline const auto& AssetProcessingPasses() const { return mAssetProcessingPasses; }
        inline const auto& AllPasses() const { return mAllPasses; }
    };

}
