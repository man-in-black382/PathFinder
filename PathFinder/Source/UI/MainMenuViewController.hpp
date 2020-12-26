#pragma once

#include "ViewController.hpp"
#include "LuminanceMeterViewController.hpp"
#include "RenderGraphViewController.hpp"
#include "ProfilerViewController.hpp"

namespace PathFinder
{
   
    class MainMenuViewController : public ViewController
    {
    public:
        void Draw() override;

    private:
        void DrawFileMenu();
        void DrawWindowMenu();

        std::shared_ptr<LuminanceMeterViewController> mLuminanceMeterVC;
        std::shared_ptr<RenderGraphViewController> mRenderGraphVC;
        std::shared_ptr<ProfilerViewController> mProfilerVC;
    };

}
