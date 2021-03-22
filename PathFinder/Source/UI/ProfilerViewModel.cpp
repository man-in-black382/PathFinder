#include "ProfilerViewModel.hpp"

namespace PathFinder
{

    void ProfilerViewModel::Import()
    {
        if (!mUpdateCooldown.Check())
            return;

        mWorkMeasurementStrings.clear();

        auto constructMeasurementString = [](const RenderDevice::PipelineMeasurement& measurement) -> std::string
        {
            std::stringstream ss;
            ss << std::setprecision(3) << std::fixed << measurement.DurationSeconds * 1000;
            return ss.str() + " ms " + measurement.Name;
        };

        mFrameMeasurementString = constructMeasurementString(Dependencies->Device->FrameMeasurement());

        for (const RenderDevice::PipelineMeasurement& measurement : Dependencies->Device->RenderPassWorkMeasurements())
        {
            mWorkMeasurementStrings.push_back(constructMeasurementString(measurement));
        }

        float marriersTime = 0.0;

        for (const RenderDevice::PipelineMeasurement& measurement : Dependencies->Device->RenderPassBarrierMeasurements())
        {
            marriersTime += measurement.DurationSeconds * 1000 * 1000;
        }

        std::stringstream ss;
        ss << std::setprecision(3) << std::fixed << marriersTime;
        mBarrierMeasurementsString = ss.str() + " us " + "Total Barriers Time";
    }

}
