#pragma once

#include "RenderPass.hpp"

#include <list>

namespace PathFinder
{

    class RenderPassExecutionGraph
    {
    public:
        void AddPass(const RenderPass* pass);

    private:
        std::list<const RenderPass *> mExecutionOrder;

    public:
        inline const auto& ExecutionOrder() const { return mExecutionOrder; }
    };

}
