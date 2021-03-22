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
        ImGui::Text(ProfilerVM->FrameMeasurement().c_str());
        ImGui::Text(ProfilerVM->BarrierMeasurements().c_str());
        ImGui::Separator();

        for (const std::string& workMeasurement : ProfilerVM->WorkMeasurements())
        {
            ImGui::Text(workMeasurement.c_str());
        }

        ImGui::End();

        ProfilerVM->Export();
    }

}
