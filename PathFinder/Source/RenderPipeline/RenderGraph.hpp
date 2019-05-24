#pragma once

#include "TextureResource.hpp"
#include "BufferResource.hpp"

namespace PathFinder
{

    class RenderGraph
    {
    public:
        using ResourceID = uint32_t;

        template <class ResourceT>
        ResourceT* GetExistingResource(ResourceState state);

        template <class BufferContentT>
        BufferResource<BufferContentT>* GetNewBufferResource(uint64_t capacity, HeapType heapType, ResourceState state);

        ColorTextureResource* GetNewColorTextureResource(
            ResourceFormat::Color dataType, ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions, ResourceState state);

        TypelessTextureResource* GetNewTypelessTextureResource(
            ResourceFormat::Color dataType, ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions, ResourceState state);

    private:

    };

}
