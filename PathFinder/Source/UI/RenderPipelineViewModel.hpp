#pragma once

#include "ViewModel.hpp"

namespace PathFinder
{
   
    class RenderPipelineViewModel : public ViewModel
    {
    public:
        PipelineSettings* RenderPipelineSettings() { return &Dependencies->RenderEngine->Settings(); }
        RenderSettings* UserRenderSettings() { return Dependencies->UserRenderSettings; }

        void SetEnableStablePowerState(bool enabled);
        void SetEnableGIDebug(bool enable);
        void SetRotateProbeRaysEachFrame(bool enable);

    private:
        bool mIsStablePowerStateEnabled = false;

    public:
        inline auto IsStablePowerStateEnabled() const { return mIsStablePowerStateEnabled; }
        inline bool RotateProbeRaysEachFrame() const { return !Dependencies->ScenePtr->GetGIManager().DoNotRotateProbeRays; }
        inline bool IsGIDebugEnabled() const { return Dependencies->ScenePtr->GetGIManager().GIDebugEnabled; }
    };

}
