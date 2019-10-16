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
        std::list<RenderPass *> mExecutionOrder;

    public:
        inline const auto& ExecutionOrder() const { return mExecutionOrder; }
        inline auto& ExecutionOrder() { return mExecutionOrder; }
    };

}
