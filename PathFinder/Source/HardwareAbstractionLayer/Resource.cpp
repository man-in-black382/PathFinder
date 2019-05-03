#include "SwapChain.hpp"
#include "Utils.h"

namespace HAL
{

    Resource::Resource(const Device& device, const ResourceFormat& format)
    {

    }

    Resource::Resource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr) 
        : mResource(existingResourcePtr) {}

    Resource::~Resource() {}

    ColorTextureResource::ColorTextureResource(
        const Device& device,
        ResourceFormat::Color dataType,
        ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions)
        : Resource(device, ResourceFormat(dataType, kind, dimensions)) {}

    TypelessTextureResource::TypelessTextureResource(
        const Device& device,
        ResourceFormat::TypelessColor dataType,
        ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions)
        : Resource(device, ResourceFormat(dataType, kind, dimensions)) {}

    DepthStencilTextureResource::DepthStencilTextureResource(
        const Device& device,
        ResourceFormat::DepthStencil dataType,
        const Geometry::Dimensions& dimensions)
        : Resource(device, ResourceFormat(dataType, ResourceFormat::TextureKind::Texture2D, dimensions)) {}

    BufferResource::BufferResource(const Device& device, ResourceFormat::Color dataType, uint64_t width)
        : Resource(device, ResourceFormat(dataType, ResourceFormat::BufferKind::Buffer, { width, 1, 1 })) {}

    TypelessBufferResource::TypelessBufferResource(const Device& device, ResourceFormat::TypelessColor dataType, uint64_t width)
        : Resource(device, ResourceFormat(dataType, ResourceFormat::BufferKind::Buffer, { width, 1, 1 })) {}

}
