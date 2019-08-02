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
        std::optional<CPUAccessibleHeapType> heapType)
        :
        TextureResource(device, ResourceFormat(dataType, kind, dimensions), initialStateMask, expectedStateMask, heapType)
    {
        mDimensions = dimensions;
        mKind = kind;
        mDataFormat = dataType;
    }

    TypelessTextureResource::TypelessTextureResource(
        const Device& device,
        ResourceFormat::TypelessColor dataType,
        ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        ResourceState initialStateMask,
        ResourceState expectedStateMask,
        std::optional<CPUAccessibleHeapType> heapType)
        :
        TextureResource(device, ResourceFormat(dataType, kind, dimensions), initialStateMask, expectedStateMask, heapType)
    {
        mDimensions = dimensions;
        mKind = kind;
    }

    DepthStencilTextureResource::DepthStencilTextureResource(
        const Device& device,
        ResourceFormat::DepthStencil dataType,
        const Geometry::Dimensions& dimensions,
        ResourceState initialStateMask,
        ResourceState expectedStateMask,
        std::optional<CPUAccessibleHeapType> heapType)
        :
        TextureResource(device, ResourceFormat(dataType, ResourceFormat::TextureKind::Texture2D, dimensions), initialStateMask, expectedStateMask, heapType)
    {
        mDimensions = dimensions;
        mKind = ResourceFormat::TextureKind::Texture2D;
    }


}
