#pragma once

#include "../HardwareAbstractionLayer/BufferResource.hpp"
#include "../HardwareAbstractionLayer/TextureResource.hpp"
#include "../Foundation/Name.hpp"
#include "../Geometry/Dimensions.hpp"

namespace PathFinder
{

    class IResourceScheduler
    {
    public:
        virtual void WillRenderToRenderTarget(Foundation::Name resourceName) = 0;

        virtual void WillRenderToDepthStencil(Foundation::Name resourceName) = 0;

        virtual void WillRenderToRenderTarget(
            Foundation::Name resourceName,
            HAL::ResourceFormat::Color dataFormat,
            HAL::ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions) = 0;

        virtual void WillRenderToRenderTarget(
            Foundation::Name resourceName,
            HAL::ResourceFormat::TypelessColor dataFormat,
            HAL::ResourceFormat::Color shaderVisisbleFormat,
            HAL::ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions) = 0;

        virtual void WillRenderToDepthStencil(
            Foundation::Name resourceName,
            HAL::ResourceFormat::DepthStencil dataFormat,
            const Geometry::Dimensions& dimensions) = 0;
    };
   
}
