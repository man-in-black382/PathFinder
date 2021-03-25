#include "RenderPipelineViewModel.hpp"

namespace PathFinder
{

    void RenderPipelineViewModel::SetEnableStablePowerState(bool enabled)
    {
        if (mIsStablePowerStateEnabled != enabled)
        {
            Dependencies->RenderEngine->Device()->EnableStablePowerState(enabled);
        }

        mIsStablePowerStateEnabled = enabled;
    }

    void RenderPipelineViewModel::SetEnableGIDebug(bool enable)
    {
        Dependencies->ScenePtr->GlobalIlluminationManager().GIDebugEnabled = enable;
    }

    void RenderPipelineViewModel::SetRotateProbeRaysEachFrame(bool enable)
    {
        Dependencies->ScenePtr->GlobalIlluminationManager().DoNotRotateProbeRays = !enable;
    }

}
