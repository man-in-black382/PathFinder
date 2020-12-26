#include "MainMenuViewController.hpp"

#include <imgui/imgui.h>

namespace PathFinder
{

    void MainMenuViewController::Draw()
    {
        if (ImGui::BeginMainMenuBar())
        {
            DrawFileMenu();
            DrawWindowMenu();
            ImGui::EndMainMenuBar();
        }
    }

    void MainMenuViewController::DrawFileMenu()
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New")) {}
            if (ImGui::MenuItem("Open", "Ctrl+O")) {}
            if (ImGui::BeginMenu("Open Recent"))
            {
                ImGui::MenuItem("fish_hat.c");
                ImGui::MenuItem("fish_hat.inl");
                ImGui::MenuItem("fish_hat.h");
                if (ImGui::BeginMenu("More.."))
                {
                    ImGui::MenuItem("Hello");
                    ImGui::MenuItem("Sailor");
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {}
            if (ImGui::MenuItem("Save As..")) {}
            ImGui::EndMenu();
        }
    }

    void MainMenuViewController::DrawWindowMenu()
    {
        if (ImGui::BeginMenu("Window"))
        {
            if (ImGui::MenuItem("Luminance Meter", nullptr, false, mLuminanceMeterVC == nullptr))
            {
                mLuminanceMeterVC = CreateViewController<LuminanceMeterViewController>();
            }

            if (ImGui::MenuItem("Render Graph", nullptr, false, mRenderGraphVC == nullptr))
            {
                mRenderGraphVC = CreateViewController<RenderGraphViewController>();
            }

            if (ImGui::MenuItem("Render Pass GPU Profiler", nullptr, false, mProfilerVC == nullptr))
            {
                mProfilerVC = CreateViewController<ProfilerViewController>();
            }

            ImGui::EndMenu();
        }
    }

}
