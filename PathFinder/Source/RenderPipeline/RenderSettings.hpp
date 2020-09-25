#pragma once

#include "../IO/Input.hpp"

namespace PathFinder
{

    struct RenderSettings
    {
        bool IsDenoiserEnabled = true;
        bool IsReprojectionHistoryDebugRenderingEnabled = false;
        bool IsDenoiserGradientDebugRenderingEnabled = false;
        bool IsDenoiserMotionDebugRenderingEnabled = false;
        bool IsDenoiserAntilagEnabled = true;
    };

    class RenderSettingsController
    {
    public:
        RenderSettingsController(Input* input);
        ~RenderSettingsController();

        void ApplyVolatileSettings();
        
        RenderSettings VolatileSettings;

    private:
        void HandleKeyUp(KeyboardKey key, const Input* input);

        RenderSettings mAppliedSettings;
        Input* mInput;

    public:
        inline const RenderSettings& AppliedSettings() const { return mAppliedSettings; }
    };

}
