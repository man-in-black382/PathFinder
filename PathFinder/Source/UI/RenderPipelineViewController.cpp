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

        ImGui::Separator();
        ImGui::Text("Antialiasing");

        ImGui::Checkbox("Enable", &VM->UserRenderSettings()->IsAntialiasingEnabled);
        ImGui::Checkbox("Enable Edge Detection", &VM->UserRenderSettings()->IsAntialiasingEdgeDetectionEnabled);
        ImGui::Checkbox("Enable Blending Weight Calculation", &VM->UserRenderSettings()->IsAntialiasingBlendingWeightCalculationEnabled);
        ImGui::Checkbox("Enable Neighborhood Blending", &VM->UserRenderSettings()->IsAntialiasingNeighborhoodBlendingEnabled);

        ImGui::Separator();
        ImGui::Text("Denoiser");

        ImGui::Checkbox("Enable", &VM->UserRenderSettings()->IsDenoiserEnabled);
        ImGui::Checkbox("Display History Reprojection", &VM->UserRenderSettings()->IsReprojectionHistoryDebugRenderingEnabled);
        ImGui::Checkbox("Display Gradient", &VM->UserRenderSettings()->IsDenoiserGradientDebugRenderingEnabled);
        ImGui::Checkbox("Display Motion", &VM->UserRenderSettings()->IsDenoiserMotionDebugRenderingEnabled);
        ImGui::Checkbox("Enable Antilag", &VM->UserRenderSettings()->IsDenoiserAntilagEnabled);

        ImGui::End();

        VM->Export();
    }

}
