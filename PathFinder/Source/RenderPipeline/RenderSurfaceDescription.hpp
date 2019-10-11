#pragma once

#include "../Foundation/Name.hpp"
#include "../Geometry/Dimensions.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"

namespace PathFinder
{

    class RenderSurfaceDescription
    {
    public:
        RenderSurfaceDescription(
            const Geometry::Dimensions& dimensions,
            HAL::ResourceFormat::Color rtFormat,
            HAL::ResourceFormat::DepthStencil dsFormat
        );

    private:
        Geometry::Dimensions mDimensions;
        HAL::ResourceFormat::Color mRTFormat;
        HAL::ResourceFormat::DepthStencil mDSFormat;

    public:
        const Geometry::Dimensions& Dimensions() const { return mDimensions; }
        const HAL::ResourceFormat::Color RenderTargetFormat() const { return mRTFormat; }
        const HAL::ResourceFormat::DepthStencil DepthStencilFormat() const { return mDSFormat; }
    };

}
