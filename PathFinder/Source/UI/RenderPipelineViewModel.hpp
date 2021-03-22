#pragma once

#include "ViewModel.hpp"

namespace PathFinder
{
   
    class RenderPipelineViewModel : public ViewModel
    {
    public:
        PipelineSettings* RenderPipelineSettings() { return &Dependencies->RenderEngine->Settings(); }
        RenderSettings* UserRenderSettings() { return Dependencies->UserRenderSettings; }

        void EnableStablePowerState(bool enabled);

    private:
        bool mIsStablePowerStateEnabled = false;

    public:
        inline auto IsStablePowerStateEnabled() const { return mIsStablePowerStateEnabled; }
    };

}
