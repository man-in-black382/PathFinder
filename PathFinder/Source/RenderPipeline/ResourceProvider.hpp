#pragma once

#include "ResourceStorage.hpp"

#include "../Foundation/Name.hpp"

namespace PathFinder
{

    class ResourceProvider
    {
    public:
        ResourceProvider(ResourceStorage* storage);
       
        uint32_t GetTextureDescriptorTableIndex(Foundation::Name resourceName);

    private:
        ResourceStorage* mResourceStorage;
    };

}
