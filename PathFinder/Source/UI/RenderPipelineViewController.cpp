#include "RenderPipelineViewController.hpp"
#include "UIManager.hpp"

namespace PathFinder
{

    void RenderPipelineViewController::OnCreated()
    {
        VM = GetViewModel<RenderPipelineViewModel>();
    }

    void RenderPipelineViewController::Draw()
    {
        VM->Import();

        ImGui::Begin("Render Pipeline", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Checkbox("Enable Memory Aliasing", &VM->RenderPipelineSettings()->IsMemoryAliasingEnabled);
        ImGui::Checkbox("Enable Async Compute", &VM->RenderPipelineSettings()->IsAsyncComputeEnabled);
        ImGui::Checkbox("Enable Split Barriers", &VM->RenderPipelineSettings()->IsSplitBarriersEnabled);

        bool isStatePowerStateEnabled = VM->IsStablePowerStateEnabled();
        if (ImGui::Checkbox("Enable Stable Power State (Windows Dev. mode required)", &isStatePowerStateEnabled))
            VM->SetEnableStablePowerState(isStatePowerStateEnabled);

        ImGui::Separator();
        ImGui::Text("Antialiasing");

        ImGui::Checkbox("Antialiasing Enabled", &VM->UserRenderSettings()->IsAntialiasingEnabled);
        ImGui::Checkbox("Enable Edge Detection", &VM->UserRenderSettings()->IsAntialiasingEdgeDetectionEnabled);
        ImGui::Checkbox("Enable Blending Weight Calculation", &VM->UserRenderSettings()->IsAntialiasingBlendingWeightCalculationEnabled);
        ImGui::Checkbox("Enable Neighborhood Blending", &VM->UserRenderSettings()->IsAntialiasingNeighborhoodBlendingEnabled);

        ImGui::Separator();
        ImGui::Text("Denoiser");

        ImGui::Checkbox("Denoiser Enabled", &VM->UserRenderSettings()->IsDenoiserEnabled);
        ImGui::Checkbox("Display History Reprojection", &VM->UserRenderSettings()->IsReprojectionHistoryDebugRenderingEnabled);
        ImGui::Checkbox("Display Gradient", &VM->UserRenderSettings()->IsDenoiserGradientDebugRenderingEnabled);
        ImGui::Checkbox("Display Motion", &VM->UserRenderSettings()->IsDenoiserMotionDebugRenderingEnabled);
        ImGui::Checkbox("Enable Antilag", &VM->UserRenderSettings()->IsDenoiserAntilagEnabled);

        ImGui::Separator();
        ImGui::Text("Global Illumination");

        bool giDebugEnabled = VM->IsGIDebugEnabled();
        if (ImGui::Checkbox("Draw GI Probes", &giDebugEnabled))
            VM->SetEnableGIDebug(giDebugEnabled);

        bool probeRotationEnabled = VM->RotateProbeRaysEachFrame();
        if (ImGui::Checkbox("Rotate Probe Rays Each Frame", &probeRotationEnabled))
            VM->SetRotateProbeRaysEachFrame(probeRotationEnabled);

        ImGui::End();

        VM->Export();
    }

}
