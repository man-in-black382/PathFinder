#pragma once

#include "ResourceStorage.hpp"

namespace PathFinder
{

    class ResourceProvider
    {
    public:
        ResourceProvider(ResourceStorage* storage);
       
    private:
        ResourceStorage* mResourceStorage;
    };

}
