#pragma once

#include "ViewController.hpp"
#include "LuminanceMeterViewModel.hpp"

namespace PathFinder
{
   
    class LuminanceMeterViewController : public ViewController
    {
    public:
        void OnCreated() override;
        void Draw() override;

        LuminanceMeterViewModel* LuminanceMeterVM;
    };

}
