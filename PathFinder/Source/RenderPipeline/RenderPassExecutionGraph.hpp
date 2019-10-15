#pragma once

//#include "RenderPass.hpp"
#include "../Foundation/Name.hpp"

#include <list>

namespace PathFinder
{
    class RenderPass;
    
    class RenderPassExecutionGraph
    {
    public:
        void AddPass(const RenderPass* pass);
        uint32_t IndexOfPass(const RenderPass* pass) const;
        uint32_t IndexOfPass(Foundation::Name passName) const;

    private:
        std::list<const RenderPass *> mExecutionOrder;

    public:
        inline const auto& ExecutionOrder() const { return mExecutionOrder; }
    };

}
