#include "RenderPipelineViewModel.hpp"

namespace PathFinder
{

    void RenderPipelineViewModel::EnableStablePowerState(bool enabled)
    {
        if (mIsStablePowerStateEnabled != enabled)
        {
            Dependencies->RenderEngine->Device()->EnableStablePowerState(enabled);
        }

        mIsStablePowerStateEnabled = enabled;
    }

}
