#include "GPUDataInspectorViewController.hpp"

namespace PathFinder
{

    void GPUDataInspectorViewController::OnCreated()
    {
        VM = GetViewModel<GPUDataInspectorViewModel>();
    }

    void GPUDataInspectorViewController::Draw()
    {
        VM->Import();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2{ 300.0f, 20.0f });
        ImGui::Begin("GPU Data Inspector", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        for (const GPUDataInspectorViewModel::InspectionDisplayData& displayData : VM->InspectionResults())
        {
            if (ImGui::TreeNode(displayData.PassName.c_str()))
            {
                for (const std::string& variable : displayData.Variables)
                {
                    ImGui::Text(variable.c_str());
                }

                ImGui::TreePop();
            }
        }
        
        if (VM->InspectionResults().empty())
            ImGui::Text("No Render Passes Provide Inspection Data");

        ImGui::End();
        ImGui::PopStyleVar();

        VM->Export();
    }

}
