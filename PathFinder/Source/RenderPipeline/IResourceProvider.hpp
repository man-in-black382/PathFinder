#pragma once

#include "../HardwareAbstractionLayer/BufferResource.hpp"
#include "../HardwareAbstractionLayer/TextureResource.hpp"
#include "../HardwareAbstractionLayer/Descriptor.hpp"
#include "../Foundation/Name.hpp"
#include "../Geometry/Dimensions.hpp"

#include "ResourceView.hpp"

namespace PathFinder
{

    class IResourceProvider
    {
    public:
        virtual ResourceView<HAL::RTDescriptor> GetRenderTarget(Foundation::Name resourceName) = 0;
        virtual ResourceView<HAL::RTDescriptor> GetBackBuffer() = 0;
        virtual ResourceView<HAL::DSDescriptor> GetDepthStencil(Foundation::Name resourceName) = 0;
    };

}
