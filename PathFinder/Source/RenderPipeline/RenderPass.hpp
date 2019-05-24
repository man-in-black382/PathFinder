#pragma once

namespace PathFinder
{

    class RenderGraph;

    class RenderPass
    {
    public:
        void RequestInputResources(const RenderGraph& renderGraph);
        void ProvideOutputResources(const RenderGraph& renderGraph);
        void Render(const RenderGraph& renderGraph);
    };

}
