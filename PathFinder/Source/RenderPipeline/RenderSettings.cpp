#include "RenderSettings.hpp"

namespace PathFinder
{

    RenderSettingsController::RenderSettingsController(Scene* scene)
        : mScene{ scene } {}

    void RenderSettingsController::SetEnabled(bool enabled)
    {
        mIsEnabled = enabled;
    }

    void RenderSettingsController::ApplyVolatileSettings()
    {
        mAppliedSettings = VolatileSettings;
        mAppliedSettings.GlobalIlluminationSettings = mScene->GlobalIlluminationManager().ProbeField;
        mAppliedSettings.IsGIDebugEnabled = mScene->GlobalIlluminationManager().GIDebugEnabled;
        mAppliedSettings.DoNotRotateGIProbeRays = mScene->GlobalIlluminationManager().DoNotRotateProbeRays;
    }

}
