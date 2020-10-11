#pragma once

#include <Foundation/Name.hpp>
#include <Geometry/Dimensions.hpp>
#include <HardwareAbstractionLayer/ResourceFormat.hpp>

#include <glm/vec2.hpp>

namespace PathFinder
{

    class RenderSurfaceDescription
    {
    public:
        RenderSurfaceDescription(
            const Geometry::Dimensions& dimensions,
            HAL::ColorFormat rtFormat,
            HAL::DepthStencilFormat dsFormat
        );

        glm::uvec2 DispatchDimensionsForGroupSize(uint32_t groupSizeX, uint32_t groupSizeY) const;

    private:
        Geometry::Dimensions mDimensions;
        HAL::ColorFormat mRTFormat;
        HAL::DepthStencilFormat mDSFormat;

    public:
        const Geometry::Dimensions& Dimensions() const { return mDimensions; }
        const HAL::ColorFormat RenderTargetFormat() const { return mRTFormat; }
        const HAL::DepthStencilFormat DepthStencilFormat() const { return mDSFormat; }
    };

}
