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

        if (ImPlot::BeginPlot("Histogram", "Luminance", "Frequency", ImVec2(-1, 0), ImPlotFlags_None, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations))
        {
            ImPlot::SetLegendLocation(ImPlotLocation_South, ImPlotOrientation_Horizontal);
            ImPlot::PushStyleColor(ImPlotCol_Fill, { 1.0, 0.0, 0.0, 1.0 });

            float width = 1.0;
            ImPlot::PlotBars("R", meter->LuminanceBins().data(), meter->HistogramBinCount(), width, 0.0);
            ImPlot::PopStyleColor(1);
           /* ImPlot::PlotBars("G", final, 10, 0.02, 0);
            ImPlot::PlotBars("B", grade, 10, 0.02, 0.01);*/

            //PlotBars(const char* label_id, const T * values, int count, double width = 0.67, double shift = 0, int offset = 0, int stride = sizeof(T));

            ImPlot::EndPlot();
        }

        ImGui::End();
    }

}
