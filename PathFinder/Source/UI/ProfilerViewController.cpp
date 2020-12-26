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

        for (int i = 0; i < ProfilerVM->Measurements().size(); i++)
        {
            const RenderDevice::PipelineMeasurement& measurement = ProfilerVM->Measurements()[i];
            float milliseconds = measurement.DurationSeconds * 1000;

            std::stringstream ss;
            ss << std::setprecision(3) << std::fixed << milliseconds;
            std::string nodeText = ss.str() + " ms " + measurement.Name;

            if (ImGui::TreeNodeEx(nodeText.c_str(), ImGuiTreeNodeFlags_Leaf))
            {
                ImGui::TreePop();
            }
        }

        ImGui::End();
    }

}
