#include "TextureResource.hpp"

namespace HAL
{

    ColorTexture::ColorTexture(
        const Device& device,
        ResourceFormat::Color dataType,
        ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        const ClearValue& optimizedClearValue,
        ResourceState initialStateMask,
        ResourceState expectedStateMask,
        std::optional<CPUAccessibleHeapType> heapType)
        :
        TextureResource(device, ResourceFormat(dataType, kind, dimensions), initialStateMask, expectedStateMask, optimizedClearValue, heapType)
    {
        mDimensions = dimensions;
        mKind = kind;
        mDataFormat = dataType;
    }

    TypelessTexture::TypelessTexture(
        const Device& device,
        ResourceFormat::TypelessColor dataType,
        ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        const ClearValue& optimizedClearValue,
        ResourceState initialStateMask,
        ResourceState expectedStateMask,
        std::optional<CPUAccessibleHeapType> heapType)
        :
        TextureResource(device, ResourceFormat(dataType, kind, dimensions), initialStateMask, expectedStateMask, optimizedClearValue, heapType)
    {
        mDimensions = dimensions;
        mKind = kind;
    }

    DepthStencilTexture::DepthStencilTexture(
        const Device& device,
        ResourceFormat::DepthStencil dataType,
        const Geometry::Dimensions& dimensions,
        const ClearValue& optimizedClearValue,
        ResourceState initialStateMask,
        ResourceState expectedStateMask,
        std::optional<CPUAccessibleHeapType> heapType)
        :
        TextureResource(device, ResourceFormat(dataType, ResourceFormat::TextureKind::Texture2D, dimensions), initialStateMask, expectedStateMask, optimizedClearValue, heapType)
    {
        mDimensions = dimensions;
        mKind = ResourceFormat::TextureKind::Texture2D;
    }

    bool TextureResource::IsArray() const
    {
        switch (mKind)
        {
        case ResourceFormat::TextureKind::Texture1D: 
        case ResourceFormat::TextureKind::Texture2D: 
            return mDimensions.Depth > 1;

        default:
            return false;
        }
    }

}
