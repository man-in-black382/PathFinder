#pragma once

#include "ViewModel.hpp"

#include <Foundation/Cooldown.hpp>

namespace PathFinder
{
   
    class ProfilerViewModel : public ViewModel
    {
    public:
        void Import() override;

    private:
        std::vector<std::string> mWorkMeasurementStrings;
        std::string mBarrierMeasurementsString;
        std::string mFrameMeasurementString;
        Foundation::Cooldown mUpdateCooldown{ 0.075 };

    public:
        inline const auto& WorkMeasurements() const { return mWorkMeasurementStrings; }
        inline const std::string& BarrierMeasurements() const { return mBarrierMeasurementsString; }
        inline const std::string& FrameMeasurement() const { return mFrameMeasurementString; }
    };

}
