#pragma once

#include "PipelineResourceStorage.hpp"

#include "../Foundation/Name.hpp"

namespace PathFinder
{

    class ResourceProvider
    {
    public:
        ResourceProvider(PipelineResourceStorage* storage);
       
        uint32_t GetTextureDescriptorTableIndex(Foundation::Name resourceName);

    private:
        PipelineResourceStorage* mResourceStorage;
    };

}
