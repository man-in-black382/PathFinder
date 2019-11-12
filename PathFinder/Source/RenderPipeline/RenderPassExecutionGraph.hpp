#pragma once

#include "../Foundation/Name.hpp"

#include <list>

namespace PathFinder
{
    class RenderPass;
    
    class RenderPassExecutionGraph
    {
    public:
        void AddPass(RenderPass* pass);
        uint32_t IndexOfPass(const RenderPass* pass) const;
        uint32_t IndexOfPass(Foundation::Name passName) const;

    private:
        std::list<RenderPass*> mDefaultPasses;
        std::list<RenderPass*> mOneTimePasses;
        std::list<RenderPass*> mAllPasses;

    public:
        inline const auto& DefaultPasses() const { return mDefaultPasses; }
        inline auto& DefaultPasses() { return mDefaultPasses; }
        inline const auto& OneTimePasses() const { return mOneTimePasses; }
        inline auto& OneTimePasses() { return mOneTimePasses; }
        inline const auto& AllPasses() const { return mAllPasses; }
        inline auto& AllPasses() { return mAllPasses; }
    };

}
