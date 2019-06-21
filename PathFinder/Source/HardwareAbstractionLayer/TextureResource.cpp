#include "TextureResource.hpp"

namespace HAL
{

    ColorTextureResource::ColorTextureResource(
        const Device& device,
        ResourceFormat::Color dataType,
        ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        ResourceState initialStateMask,
        ResourceState expectedStateMask,
        HeapType heapType)
        :
        TextureResource(device, ResourceFormat(dataType, kind, dimensions), initialStateMask, expectedStateMask, heapType)
    {
        mDimensions = dimensions;
    }

    TypelessTextureResource::TypelessTextureResource(
        const Device& device,
        ResourceFormat::TypelessColor dataType,
        ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        ResourceState initialStateMask,
        ResourceState expectedStateMask,
        HeapType heapType)
        :
        TextureResource(device, ResourceFormat(dataType, kind, dimensions), initialStateMask, expectedStateMask, heapType)
    {
        mDimensions = dimensions;
    }

    DepthStencilTextureResource::DepthStencilTextureResource(
        const Device& device,
        ResourceFormat::DepthStencil dataType,
        const Geometry::Dimensions& dimensions,
        ResourceState initialStateMask,
        ResourceState expectedStateMask,
        HeapType heapType)
        :
        TextureResource(device, ResourceFormat(dataType, ResourceFormat::TextureKind::Texture2D, dimensions), initialStateMask, expectedStateMask, heapType)
    {
        mDimensions = dimensions;
    }


}
