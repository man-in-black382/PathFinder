#pragma once

#include "ViewModel.hpp"

namespace PathFinder
{
   
    class RenderGraphViewModel : public ViewModel
    {
    public:
        void Import() override;
        void Export() override;

    private:
    };

}
