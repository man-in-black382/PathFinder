#pragma once

#include "ViewController.hpp"
#include "RenderPipelineViewModel.hpp"

namespace PathFinder
{
   
    class RenderPipelineViewController : public ViewController
    {
    public:
        void Draw() override;
        void OnCreated() override;

        RenderPipelineViewModel* VM;
    };

}
