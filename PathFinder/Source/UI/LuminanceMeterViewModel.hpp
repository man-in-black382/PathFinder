#pragma once

#include "ViewModel.hpp"

#include <Scene/LuminanceMeter.hpp>

namespace PathFinder
{
   
    class LuminanceMeterViewModel : public ViewModel
    {
    public:
        void OnCreated() override;
        void Import() override;
        void Export() override;

    private:
        LuminanceMeter* mLuminanceMeter = nullptr;

    public:
        inline const LuminanceMeter* LumMeter() const { return mLuminanceMeter; }
    };

}
