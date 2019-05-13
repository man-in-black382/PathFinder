#include "Resource.hpp"
#include "Utils.h"
#include "ResourceHelpers.hpp"

#include "../Foundation/Visitor.hpp"

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

    ResourceTransitionBarrier ColorTextureResource::BarrierToStates(std::initializer_list<ReadTextureResourceState> states)
    {
        return ResourceTransitionBarrier(D3D12_RESOURCE_STATE_COMMON, D3DResourceStatesFromVariantList(states), this);
    }

    ResourceTransitionBarrier ColorTextureResource::BarrierToState(ReadTextureResourceState state)
    {
        return ResourceTransitionBarrier(D3D12_RESOURCE_STATE_COMMON, D3DResourceStatesFromVariant(state), this);
    }

    ResourceTransitionBarrier ColorTextureResource::BarrierToState(WriteTextureResourceState state)
    {
        return ResourceTransitionBarrier(D3D12_RESOURCE_STATE_COMMON, D3DResourceStatesFromVariant(state), this);
    }

    ResourceTransitionBarrier ColorTextureResource::BarrierToState(ReadWriteTextureResourceState state)
    {
        return ResourceTransitionBarrier(D3D12_RESOURCE_STATE_COMMON, D3DResourceStatesFromVariant(state), this);
    }



    TypelessTextureResource::TypelessTextureResource(
        const Device& device,
        ResourceFormat::TypelessColor dataType,
        ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        HeapType heapType)
        : Resource(device, ResourceFormat(dataType, kind, dimensions), heapType, D3D12_RESOURCE_STATE_RENDER_TARGET) {}

    ResourceTransitionBarrier TypelessTextureResource::BarrierToStates(std::initializer_list<ReadTextureResourceState> states)
    {
        return ResourceTransitionBarrier(D3D12_RESOURCE_STATE_COMMON, D3DResourceStatesFromVariantList(states), this);
    }

    ResourceTransitionBarrier TypelessTextureResource::BarrierToState(ReadTextureResourceState state)
    {
        return ResourceTransitionBarrier(D3D12_RESOURCE_STATE_COMMON, D3DResourceStatesFromVariant(state), this);
    }

    ResourceTransitionBarrier TypelessTextureResource::BarrierToState(WriteTextureResourceState state)
    {
        return ResourceTransitionBarrier(D3D12_RESOURCE_STATE_COMMON, D3DResourceStatesFromVariant(state), this);
    }

    ResourceTransitionBarrier TypelessTextureResource::BarrierToState(ReadWriteTextureResourceState state)
    {
        return ResourceTransitionBarrier(D3D12_RESOURCE_STATE_COMMON, D3DResourceStatesFromVariant(state), this);
    }



    

    DepthStencilTextureResource::DepthStencilTextureResource(
        const Device& device,
        ResourceFormat::DepthStencil dataType,
        const Geometry::Dimensions& dimensions,
        HeapType heapType)
        : Resource(device, ResourceFormat(dataType, ResourceFormat::TextureKind::Texture2D, dimensions), heapType, D3D12_RESOURCE_STATE_DEPTH_WRITE) {}

    ResourceTransitionBarrier DepthStencilTextureResource::BarrierToStates(std::initializer_list<ReadDepthStencilTextureResourceState> states)
    {
        return ResourceTransitionBarrier(D3D12_RESOURCE_STATE_COMMON, D3DResourceStatesFromVariantList(states), this);
    }
    
    ResourceTransitionBarrier DepthStencilTextureResource::BarrierToState(ReadDepthStencilTextureResourceState state)
    {
        return ResourceTransitionBarrier(D3D12_RESOURCE_STATE_COMMON, D3DResourceStatesFromVariant(state), this);
    }

    ResourceTransitionBarrier DepthStencilTextureResource::BarrierToState(WriteDepthStencilTextureResourceState state)
    {
        return ResourceTransitionBarrier(D3D12_RESOURCE_STATE_COMMON, D3DResourceStatesFromVariant(state), this);
    }

    ResourceTransitionBarrier DepthStencilTextureResource::BarrierToState(ReadWriteDepthStencilTextureResourceState state)
    {
        return ResourceTransitionBarrier(D3D12_RESOURCE_STATE_COMMON, D3DResourceStatesFromVariant(state), this);
    }



    

    ColorBufferResource::ColorBufferResource(const Device& device, ResourceFormat::Color dataType, uint64_t width, HeapType heapType)
        : Resource(device, ResourceFormat(dataType, ResourceFormat::BufferKind::Buffer, { width, 1, 1 }), heapType, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) {}



    TypelessBufferResource::TypelessBufferResource(const Device& device, ResourceFormat::TypelessColor dataType, uint64_t width, HeapType heapType)
        : Resource(device, ResourceFormat(dataType, ResourceFormat::BufferKind::Buffer, { width, 1, 1 }), heapType, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER) {}

}
