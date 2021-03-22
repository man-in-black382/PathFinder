#pragma once

#include <IO/Input.hpp>
#include <glm/vec2.hpp>
#include <Scene/Scene.hpp>

#include "PipelineSettings.hpp"

namespace PathFinder
{

    struct RenderSettings
    {
        bool IsDenoiserEnabled = true;
        bool IsReprojectionHistoryDebugRenderingEnabled = false;
        bool IsDenoiserGradientDebugRenderingEnabled = false;
        bool IsDenoiserMotionDebugRenderingEnabled = false;
        bool IsDenoiserAntilagEnabled = true;
        bool IsAntialiasingEnabled = true;
        bool IsAntialiasingEdgeDetectionEnabled = true;
        bool IsAntialiasingBlendingWeightCalculationEnabled = true;
        bool IsAntialiasingNeighborhoodBlendingEnabled = true;

        IrradianceField GlobalIlluminationSettings;
        PipelineSettings RenderPipelineSettings;
    };

    class RenderSettingsController
    {
    public:
        RenderSettingsController(Scene* scene);

        void SetEnabled(bool enabled);
        void ApplyVolatileSettings();
        
        RenderSettings VolatileSettings;

    private:
        RenderSettings mAppliedSettings;
        Scene* mScene;

        bool mIsEnabled = true;

    public:
        inline const RenderSettings& AppliedSettings() const { return mAppliedSettings; }
    };

}
