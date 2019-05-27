#pragma once

#include "../HardwareAbstractionLayer/BufferResource.hpp"
#include "../HardwareAbstractionLayer/TextureResource.hpp"

#include "../Geometry/Dimensions.hpp"

namespace PathFinder
{

    class IRenderPassScheduler
    {
    public:
        virtual void WillRenderToRenderTarget(HAL::ResourceFormat::Color dataFormat, HAL::ResourceFormat::TextureKind kind, const Geometry::Dimensions& dimensions) = 0;
        virtual void WillRenderToRenderTarget(HAL::ResourceFormat::TypelessColor dataFormat, HAL::ResourceFormat::TextureKind kind, const Geometry::Dimensions& dimensions) = 0;
        virtual void WillRenderToDepthStencil(HAL::ResourceFormat::DepthStencil dataFormat, const Geometry::Dimensions& dimensions) = 0;
    };
   
}
