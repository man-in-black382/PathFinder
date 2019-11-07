#include "RenderSurfaceDescription.hpp"

namespace PathFinder
{

    RenderSurfaceDescription::RenderSurfaceDescription(const Geometry::Dimensions& dimensions, HAL::ResourceFormat::Color rtFormat, HAL::ResourceFormat::DepthStencil dsFormat)
        : mDimensions(dimensions), mRTFormat(rtFormat), mDSFormat(dsFormat) {}

    glm::uvec2 RenderSurfaceDescription::DispatchDimensionsForGroupSize(uint32_t groupSizeX, uint32_t groupSizeY) const
    {
        float x = ceilf((float)mDimensions.Width / 32);
        float y = ceilf((float)mDimensions.Height / 32);
        return { x, y };
    }

}
