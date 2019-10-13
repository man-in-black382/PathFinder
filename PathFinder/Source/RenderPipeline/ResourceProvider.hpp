#pragma once

#include "PipelineResourceStorage.hpp"

#include "../Foundation/Name.hpp"

namespace PathFinder
{

    class ResourceProvider
    {
    public:
        ResourceProvider(const PipelineResourceStorage* storage);
       
        uint32_t GetTextureDescriptorTableIndex(Foundation::Name resourceName) const;

    private:
        const PipelineResourceStorage* mResourceStorage;
    };

}
