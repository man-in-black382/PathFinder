#pragma once

#include "ViewModel.hpp"

namespace PathFinder
{
   
    class MainMenuViewModel : public ViewModel
    {
    public:
        void Import() override;
        void Export() override;

    private:
    };

}
