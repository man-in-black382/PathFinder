#include "RenderSettings.hpp"

namespace PathFinder
{

    RenderSettingsController::RenderSettingsController(Input* input, Scene* scene)
        : mInput{ input }, mScene{ scene }
    {
        mInput->KeyUpEvent() += { "RenderSettingsController.Key.Up", this, &RenderSettingsController::HandleKeyUp };
    }

    RenderSettingsController::~RenderSettingsController()
    {
        mInput->KeyUpEvent() -= "RenderSettingsController.Key.Up";
    }

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

    void RenderSettingsController::HandleKeyUp(KeyboardKey key, const KeyboardKeyInfo& info, const Input* input)
    {
        if (!mIsEnabled)
            return;

        switch (key)
        {
        case KeyboardKey::R: VolatileSettings.IsDenoiserEnabled = !VolatileSettings.IsDenoiserEnabled; break;
        case KeyboardKey::T: VolatileSettings.IsReprojectionHistoryDebugRenderingEnabled = !VolatileSettings.IsReprojectionHistoryDebugRenderingEnabled; break;
        case KeyboardKey::Y: VolatileSettings.IsDenoiserGradientDebugRenderingEnabled = !VolatileSettings.IsDenoiserGradientDebugRenderingEnabled; break;
        case KeyboardKey::U: VolatileSettings.IsDenoiserMotionDebugRenderingEnabled = !VolatileSettings.IsDenoiserMotionDebugRenderingEnabled; break;
        case KeyboardKey::I: VolatileSettings.IsDenoiserAntilagEnabled = !VolatileSettings.IsDenoiserAntilagEnabled; break;
        }
    }

}
