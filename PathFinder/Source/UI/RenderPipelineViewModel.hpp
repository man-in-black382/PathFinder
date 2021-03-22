#pragma once

#include "ViewModel.hpp"

namespace PathFinder
{
   
    class RenderPipelineViewModel : public ViewModel
    {
    public:
        PipelineSettings* RenderPipelineSettings() { return Dependencies->RenderPipelineSettings; }
        RenderSettings* UserRenderSettings() { return Dependencies->UserRenderSettings; }
    };

}
