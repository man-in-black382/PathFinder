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
        std::vector<RenderDevice::PipelineMeasurement> mMeasurements;
        Foundation::Cooldown mUpdateCooldown{ 0.15 };

    public:
        inline const auto& Measurements() const { return mMeasurements; }
    };

}
