#include "RenderSurface.hpp"

namespace PathFinder
{

    RenderSurface::RenderSurface(const Geometry::Dimensions& dimensions, HAL::ResourceFormat::Color rtFormat, HAL::ResourceFormat::DepthStencil dsFormat)
        : mDimensions(dimensions), mRTFormat(rtFormat), mDSFormat(dsFormat) {}

}
