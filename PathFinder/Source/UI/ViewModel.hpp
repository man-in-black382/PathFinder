#pragma once

#include "UIDependencies.hpp"

#include <robinhood/robin_hood.h>
#include <typeindex>

namespace PathFinder
{

    class ViewModel
    {
    public:
        virtual void Import() {};
        virtual void Export() {};
        virtual void OnCreated() {};

        UIDependencies* Dependencies;
    };

}
