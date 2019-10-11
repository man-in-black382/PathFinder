#include "RenderSurfaceDescription.hpp"

namespace PathFinder
{

    RenderSurfaceDescription::RenderSurfaceDescription(const Geometry::Dimensions& dimensions, HAL::ResourceFormat::Color rtFormat, HAL::ResourceFormat::DepthStencil dsFormat)
        : mDimensions(dimensions), mRTFormat(rtFormat), mDSFormat(dsFormat) {}

}
