#include "Resource.hpp"
#include "Utils.h"
#include "ResourceHelpers.hpp"

#include "../Foundation/Visitor.hpp"

namespace HAL
{

    ColorTextureResource::ColorTextureResource(
        const Device& device,
        ResourceFormat::Color dataType,
        ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        HeapType heapType)
        : Resource(device, ResourceFormat(dataType, kind, dimensions), heapType, CommonResourceState) {}


    TypelessTextureResource::TypelessTextureResource(
        const Device& device,
        ResourceFormat::TypelessColor dataType,
        ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        HeapType heapType)
        : Resource(device, ResourceFormat(dataType, kind, dimensions), heapType, CommonResourceState) {}

    

    DepthStencilTextureResource::DepthStencilTextureResource(
        const Device& device,
        ResourceFormat::DepthStencil dataType,
        const Geometry::Dimensions& dimensions,
        HeapType heapType)
        : Resource(device, ResourceFormat(dataType, ResourceFormat::TextureKind::Texture2D, dimensions), heapType, DepthWriteResourceState) {}

    

    ColorBufferResource::ColorBufferResource(const Device& device, ResourceFormat::Color dataType, uint64_t width, HeapType heapType)
        : Resource(device, ResourceFormat(dataType, ResourceFormat::BufferKind::Buffer, { width, 1, 1 }), heapType, VertexAndConstantResourceState) {}



    /*TypelessBufferResource::TypelessBufferResource(const Device& device, ResourceFormat::TypelessColor dataType, uint64_t width, HeapType heapType)
        : Resource(device, ResourceFormat(dataType, ResourceFormat::BufferKind::Buffer, { width, 1, 1 }), heapType, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) {}*/

}
