#include "LuminanceMeterViewController.hpp"

#include <implot/implot.h>

namespace PathFinder
{

    void LuminanceMeterViewController::OnCreated()
    {
        LuminanceMeterVM = GetViewModel<LuminanceMeterViewModel>();
    }

    void LuminanceMeterViewController::Draw()
    {
        LuminanceMeterVM->Import();

        const LuminanceMeter* meter = LuminanceMeterVM->LumMeter();

        if (!meter || !meter->LuminanceBins().size())
            return;

        ImGui::SetNextWindowSize({ 800, 335 }, ImGuiCond_Once);
        ImGui::Begin("Luminance Meter");
        ImPlot::StyleColorsDark();
        ImPlot::SetNextPlotLimits(-1, meter->HistogramBinCount() + 1, 0, meter->MaxLuminanceBinSize(), ImGuiCond_Always);
        ImPlot::SetNextPlotTicksX(0.0, meter->HistogramBinCount(), meter->HistogramBinCount());

        if (ImPlot::BeginPlot("Histogram", "Luminance Bins", "Pixel Count", ImVec2(-1, 0), ImPlotFlags_None, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations))
        {
            ImPlot::SetLegendLocation(ImPlotLocation_South, ImPlotOrientation_Horizontal);
            ImPlot::PushStyleColor(ImPlotCol_Fill, { 0.8, 0.2, 0.2, 1.0 });

            float width = 1.0;
            ImPlot::PlotBars("", meter->LuminanceBins().data(), meter->HistogramBinCount(), width, 0.0);
            ImPlot::PopStyleColor(1);
            ImPlot::EndPlot();
        }

        ImGui::End();
    }

}
