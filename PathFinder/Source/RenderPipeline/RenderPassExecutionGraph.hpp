#pragma once

#include "../Foundation/Name.hpp"

#include <list>

namespace PathFinder
{
    class RenderPass;
    
    class RenderPassExecutionGraph
    {
    public:
        //struct RenderPass

        void AddPass(RenderPass* pass);
        uint32_t IndexOfPass(const RenderPass* pass) const;
        uint32_t IndexOfPass(Foundation::Name passName) const;

    private:
        std::list<RenderPass*> mDefaultPasses;
        std::list<RenderPass*> mSetupPasses;
        std::list<RenderPass*> mAssetProcessingPasses;
        std::list<RenderPass*> mAllPasses;

    public:
        inline const auto& DefaultPasses() const { return mDefaultPasses; }
        inline auto& DefaultPasses() { return mDefaultPasses; }
        inline const auto& SetupPasses() const { return mSetupPasses; }
        inline auto& SetupPasses() { return mSetupPasses; }
        inline const auto& AssetProcessingPasses() const { return mAssetProcessingPasses; }
        inline auto& AssetProcessingPasses() { return mAssetProcessingPasses; }
        inline const auto& AllPasses() const { return mAllPasses; }
        inline auto& AllPasses() { return mAllPasses; }
    };

}
