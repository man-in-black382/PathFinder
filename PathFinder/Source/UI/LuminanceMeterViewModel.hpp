#pragma once

#include "ViewModel.hpp"

#include <Scene/LuminanceMeter.hpp>

namespace PathFinder
{
   
    class LuminanceMeterViewModel : public ViewModel
    {
    public:
        void SetLuminanceMeter(LuminanceMeter* meter);
        void Import() override;
        void Export() override;

    private:
        LuminanceMeter* mLuminanceMeter = nullptr;
    };

}
