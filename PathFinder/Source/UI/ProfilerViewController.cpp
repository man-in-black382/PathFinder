#include "ProfilerViewController.hpp"

#include <implot/implot.h>

namespace PathFinder
{

    void ProfilerViewController::OnCreated()
    {
        ProfilerVM = GetViewModel<ProfilerViewModel>();
    }

    void ProfilerViewController::Draw()
    {
        ProfilerVM->Import();

        ImGui::Begin("GPU Profiler", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        auto constructMeasurementString = [](const RenderDevice::PipelineMeasurement& measurement) -> std::string
        {
            std::stringstream ss;
            ss << std::setprecision(3) << std::fixed << measurement.DurationSeconds * 1000;
            return ss.str() + " ms " + measurement.Name;
        };

        if (ImGui::TreeNodeEx(constructMeasurementString(ProfilerVM->FrameMeasurement()).c_str(), ImGuiTreeNodeFlags_Leaf))
        {
            ImGui::TreePop();
        }

        ImGui::Separator();

        for (int i = 0; i < ProfilerVM->Measurements().size(); i++)
        {
            const RenderDevice::PipelineMeasurement& measurement = ProfilerVM->Measurements()[i];

            if (ImGui::TreeNodeEx(constructMeasurementString(measurement).c_str(), ImGuiTreeNodeFlags_Leaf))
            {
                ImGui::TreePop();
            }
        }

        ImGui::End();
    }

}
