#pragma once

#include <array>

namespace PathFinder
{

    struct BlurCBContent
    {
        static const int MaximumRadius = 64;

        std::array<float, MaximumRadius> Weights;

        uint32_t BlurRadius;
        uint32_t InputTextureIndex;
        uint32_t OutputTextureIndex;
        
    };

    class BlurRenderHelper
    { 
    public: 
        BlurRenderHelper();
        ~BlurRenderHelper() = default;

        //virtual void Render(RenderContext<RenderPassContentMediator>* context) override;
    };

}
