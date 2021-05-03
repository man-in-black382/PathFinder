#include "RenderSettings.hpp"

namespace PathFinder
{

    void RenderSettingsController::SetEnabled(bool enabled)
    {
        mIsEnabled = enabled;
    }

    void RenderSettingsController::ApplyVolatileSettings(const Scene& scene)
    {
        mAppliedSettings = VolatileSettings;
        mAppliedSettings.GlobalIlluminationSettings = scene.GetGIManager().ProbeField;
        mAppliedSettings.IsGIDebugEnabled = scene.GetGIManager().GIDebugEnabled;
        mAppliedSettings.DoNotRotateGIProbeRays = scene.GetGIManager().DoNotRotateProbeRays;
    }

}
