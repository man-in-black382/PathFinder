#include "SwapChain.hpp"
#include "Utils.h"

namespace HAL
{

    Resource::Resource(const Device& device, const ResourceFormat& format, HeapType heapType, D3D12_RESOURCE_STATES initialStates)
    {
        D3D12_HEAP_PROPERTIES heapProperties;
        D3D12_RESOURCE_STATES resourceStates = initialStates;

        switch (heapType) {
        case HeapType::Default:
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
            break;
        case HeapType::Upload:
            heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
            resourceStates |= D3D12_RESOURCE_STATE_GENERIC_READ;
            break;
        case HeapType::Readback:
            heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
            break;
        }

        D3D12_CLEAR_VALUE clearValue;

        ThrowIfFailed(device.D3DPtr()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &format.D3DResourceDescription(),
            resourceStates,
            nullptr,
            IID_PPV_ARGS(&mResource)
        ));
    }

    Resource::Resource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr)
        : mResource(existingResourcePtr) {}

    Resource::~Resource() {}

    ColorTextureResource::ColorTextureResource(
        const Device& device,
        ResourceFormat::Color dataType,
        ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        HeapType heapType)
        : Resource(device, ResourceFormat(dataType, kind, dimensions), heapType, D3D12_RESOURCE_STATE_RENDER_TARGET) {}

    TypelessTextureResource::TypelessTextureResource(
        const Device& device,
        ResourceFormat::TypelessColor dataType,
        ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        HeapType heapType)
        : Resource(device, ResourceFormat(dataType, kind, dimensions), heapType, D3D12_RESOURCE_STATE_RENDER_TARGET) {}

    DepthStencilTextureResource::DepthStencilTextureResource(
        const Device& device,
        ResourceFormat::DepthStencil dataType,
        const Geometry::Dimensions& dimensions,
        HeapType heapType)
        : Resource(device, ResourceFormat(dataType, ResourceFormat::TextureKind::Texture2D, dimensions), heapType, D3D12_RESOURCE_STATE_DEPTH_WRITE) {}

    ColorBufferResource::ColorBufferResource(const Device& device, ResourceFormat::Color dataType, uint64_t width, HeapType heapType)
        : Resource(device, ResourceFormat(dataType, ResourceFormat::BufferKind::Buffer, { width, 1, 1 }), heapType, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) {}

    TypelessBufferResource::TypelessBufferResource(const Device& device, ResourceFormat::TypelessColor dataType, uint64_t width, HeapType heapType)
        : Resource(device, ResourceFormat(dataType, ResourceFormat::BufferKind::Buffer, { width, 1, 1 }), heapType, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) {}

}
