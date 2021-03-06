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

        ImGui::End();

        VM->Export();
    }

}
