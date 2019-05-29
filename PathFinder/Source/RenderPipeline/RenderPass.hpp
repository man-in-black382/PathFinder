#pragma once

#include "../Foundation/Name.hpp"

#include "RenderPassScheduler.hpp"

namespace PathFinder
{

    class RenderPass
    {
    public:
        RenderPass(Foundation::Name name);

        virtual void ScheduleResources(const IRenderPassScheduler* scheduler) = 0;
        //void Render(const RenderGraph& renderGraph);

    private:
        Foundation::Name mName;

    public:
        inline const auto& Name() const { return mName; }
    };

}
