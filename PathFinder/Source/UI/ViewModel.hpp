#pragma once

#include "UIDependencies.hpp"

#include <robinhood/robin_hood.h>
#include <typeindex>

namespace PathFinder
{

    class ViewModel
    {
    public:
        virtual void Import() = 0;
        virtual void Export() = 0;
        virtual void OnCreated() {};

        UIDependencies* Dependencies;
    };

}
