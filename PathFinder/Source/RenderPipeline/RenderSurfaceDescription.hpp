#pragma once

#include "../Foundation/Name.hpp"
#include "../Geometry/Dimensions.hpp"
#include "../HardwareAbstractionLayer/ResourceFormat.hpp"

#include <glm/vec2.hpp>

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

        glm::uvec2 DispatchDimensionsForGroupSize(uint32_t groupSizeX, uint32_t groupSizeY) const;

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
