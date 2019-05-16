#include "Resource.hpp"
#include "Utils.h"

#include "../Foundation/Visitor.hpp"

namespace HAL
{
    
    Resource::Resource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr)
        : mResource(existingResourcePtr), mDescription(mResource->GetDesc()) {}

    Resource::Resource(const Device& device, const ResourceFormat& format, ResourceState initialStateMask, ResourceState expectedStateMask, HeapType heapType)
        : mInitialState(D3DResourceState(initialStateMask)), mDescription(format.D3DResourceDescription())
    {
        D3D12_HEAP_PROPERTIES heapProperties{};

        switch (heapType) {
        case HeapType::Default:
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
            break;
        case HeapType::Upload:
            heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
            mInitialState |= D3D12_RESOURCE_STATE_GENERIC_READ;
            break;
        case HeapType::Readback:
            heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
            break;
        }

        SetExpectedUsageFlags(expectedStateMask);

        D3D12_CLEAR_VALUE clearValue;

        ThrowIfFailed(device.D3DPtr()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &mDescription,
            mInitialState,
            nullptr,
            IID_PPV_ARGS(&mResource)
        ));
    }

    Resource::~Resource() {}

    void Resource::SetExpectedUsageFlags(ResourceState stateMask)
    {
        if (BitwiseEnumMaskContainsComponent(stateMask, ResourceState::RenderTarget))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }

        if (BitwiseEnumMaskContainsComponent(stateMask, ResourceState::DepthRead) ||
            BitwiseEnumMaskContainsComponent(stateMask, ResourceState::DepthWrite))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }

        if (BitwiseEnumMaskContainsComponent(stateMask, ResourceState::UnorderedAccess))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        if (!BitwiseEnumMaskContainsComponent(stateMask, ResourceState::PixelShaderAccess) &&
            !BitwiseEnumMaskContainsComponent(stateMask, ResourceState::NonPixelShaderAccess))
        {
            mDescription.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }
    }

    ColorTextureResource::ColorTextureResource(
        const Device& device,
        ResourceFormat::Color dataType,
        ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        ResourceState initialStateMask,
        ResourceState expectedStateMask,
        HeapType heapType)
        :
        Resource(device, ResourceFormat(dataType, kind, dimensions), initialStateMask, expectedStateMask, heapType) {}



    TypelessTextureResource::TypelessTextureResource(
        const Device& device,
        ResourceFormat::TypelessColor dataType,
        ResourceFormat::TextureKind kind,
        const Geometry::Dimensions& dimensions,
        ResourceState initialStateMask,
        ResourceState expectedStateMask,
        HeapType heapType)
        :
        Resource(device, ResourceFormat(dataType, kind, dimensions), initialStateMask, expectedStateMask, heapType) {}



    DepthStencilTextureResource::DepthStencilTextureResource(
        const Device& device,
        ResourceFormat::DepthStencil dataType,
        const Geometry::Dimensions& dimensions,
        ResourceState initialStateMask,
        ResourceState expectedStateMask,
        HeapType heapType)
        :
        Resource(device, ResourceFormat(dataType, ResourceFormat::TextureKind::Texture2D, dimensions), initialStateMask, expectedStateMask, heapType) {}



    ColorBufferResource::ColorBufferResource(const Device& device, ResourceFormat::Color dataType, uint64_t width, HeapType heapType)
        : Resource(device, ResourceFormat(dataType, ResourceFormat::BufferKind::Buffer, { width, 1, 1 }), ResourceState::VertexAndConstantBuffer, ResourceState::VertexAndConstantBuffer, heapType) {}

}
