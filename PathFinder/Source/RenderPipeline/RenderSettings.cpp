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
        // Gather settings from around the engine in here
        VolatileSettings.GlobalIlluminationSettings = mScene->GlobalIlluminationManager().ProbeField;

        mAppliedSettings = VolatileSettings;
    }

}
