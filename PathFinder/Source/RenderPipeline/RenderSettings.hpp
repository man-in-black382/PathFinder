#pragma once

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

        bool IsGIDebugEnabled = false;
        bool DoNotRotateGIProbeRays = false;
        bool IsGIEnabled = true;
        bool IsAOEnabled = true;

        bool IsTAAEnabled = true;
        bool IsTAAYCoCgSpaceEnabled = true;
        uint32_t TAASampleCount = 16;

        IrradianceField GlobalIlluminationSettings;
        PipelineSettings RenderPipelineSettings;
    };

    class RenderSettingsController
    {
    public:
        void SetEnabled(bool enabled);
        void ApplyVolatileSettings(const Scene& scene);
        
        RenderSettings VolatileSettings;

    private:
        RenderSettings mAppliedSettings;

        bool mIsEnabled = true;

    public:
        inline const RenderSettings* GetAppliedSettings() const { return &mAppliedSettings; }
    };

}
