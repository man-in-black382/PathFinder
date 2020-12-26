#pragma once

#include "ViewController.hpp"
#include "ProfilerViewModel.hpp"

namespace PathFinder
{
   
    class ProfilerViewController : public ViewController
    {
    public:
        void OnCreated() override;
        void Draw() override;

        ProfilerViewModel* ProfilerVM;

    private:
    };

}
