#include "TextureResource.hpp"

namespace HAL
{

    TextureResource::TextureResource(
        const Device& device, 
        ResourceFormat::FormatVariant format,
        ResourceFormat::TextureKind kind, 
        const Geometry::Dimensions& dimensions, 
        const ClearValue& optimizedClearValue, 
        ResourceState initialStateMask, 
        ResourceState expectedStateMask,
        std::optional<CPUAccessibleHeapType> heapType)
        :
        Resource(device, ResourceFormat(format, kind, dimensions), initialStateMask, expectedStateMask, optimizedClearValue, heapType),
        mDimensions{ dimensions }, mKind{ kind }, mFormat{ format }
    {

    }

    TextureResource::TextureResource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr)
        : Resource(existingResourcePtr) {}

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
