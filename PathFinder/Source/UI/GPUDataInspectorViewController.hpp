#pragma once

#include "ViewController.hpp"
#include "GPUDataInspectorViewModel.hpp"

namespace PathFinder
{
   
    class GPUDataInspectorViewController : public ViewController
    {
    public:
        void Draw() override;
        void OnCreated() override;

        GPUDataInspectorViewModel* VM;
    };

}
