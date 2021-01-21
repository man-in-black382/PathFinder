#pragma once

#include <IO/Input.hpp>
#include <glm/vec2.hpp>
#include <Scene/Scene.hpp>

namespace PathFinder
{

    struct RenderSettings
    {
        bool IsDenoiserEnabled = true;
        bool IsReprojectionHistoryDebugRenderingEnabled = false;
        bool IsDenoiserGradientDebugRenderingEnabled = false;
        bool IsDenoiserMotionDebugRenderingEnabled = false;
        bool IsDenoiserAntilagEnabled = true;

        IrradianceField GlobalIlluminationSettings;
    };

    class RenderSettingsController
    {
    public:
        RenderSettingsController(Input* input, Scene* scene);
        ~RenderSettingsController();

        void SetEnabled(bool enabled);
        void ApplyVolatileSettings();
        
        RenderSettings VolatileSettings;

    private:
        void HandleKeyUp(KeyboardKey key, const KeyboardKeyInfo& info, const Input* input);

        RenderSettings mAppliedSettings;
        Input* mInput;
        Scene* mScene;

        bool mIsEnabled = true;

    public:
        inline const RenderSettings& AppliedSettings() const { return mAppliedSettings; }
    };

}
