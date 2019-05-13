#pragma once

#include <wrl.h>
#include <dxgi.h>
#include <cstdint>
#include <d3d12.h>

#include "Device.hpp"
#include "ResourceFormat.hpp"
#include "ResourceState.hpp"
#include "ResourceBarrier.hpp"

#include "../Geometry/Dimensions.hpp"

namespace HAL
{
    class Resource {
    public:
        enum class HeapType { Default, Upload, Readback };

        Resource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr);
        virtual ~Resource() = 0;

    protected:
        Resource(const Device& device, const ResourceFormat& format, HeapType heapType, D3D12_RESOURCE_STATES initialStates /* Replace later */);

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
        D3D12_RESOURCE_DESC mDescription;
        Geometry::Dimensions mDimensions;

    public:
        inline const auto D3DPtr() const { return mResource.Get(); }
        inline const auto& D3DDescription() const { return mDescription; };
        inline const auto& Dimensions() const { return mDimensions; };
    };

    class ColorTextureResource : public Resource {
    public:
        using Resource::Resource;
        ColorTextureResource(
            const Device& device, 
            ResourceFormat::Color dataType, 
            ResourceFormat::TextureKind kind, 
            const Geometry::Dimensions& dimensions, 
            HeapType heapType = HeapType::Default
        );

        //ResourceTransitionBarrier BarrierToState(std::initializer_list<ReadTextureResourceState> states);
        //ResourceTransitionBarrier BarrierToState(WriteTextureResourceState state);
        //ResourceTransitionBarrier BarrierToState(ReadWriteTextureResourceState state);
    };

    class TypelessTextureResource : public Resource {
    public:
        using Resource::Resource;
        TypelessTextureResource(
            const Device& device,
            ResourceFormat::TypelessColor dataType,
            ResourceFormat::TextureKind kind, 
            const Geometry::Dimensions& dimensions,
            HeapType heapType = HeapType::Default
        );

    /*    ResourceTransitionBarrier BarrierToState(std::initializer_list<ReadTextureResourceState> states);
        ResourceTransitionBarrier BarrierToState(WriteTextureResourceState state);
        ResourceTransitionBarrier BarrierToState(ReadWriteTextureResourceState state);*/
    };

    class DepthStencilTextureResource : public Resource {
    public:
        using Resource::Resource;
        DepthStencilTextureResource(
            const Device& device, 
            ResourceFormat::DepthStencil dataType, 
            const Geometry::Dimensions& dimensions,
            HeapType heapType = HeapType::Default
        );

      /*  ResourceTransitionBarrier BarrierToState(std::initializer_list<ReadDepthStencilTextureResourceState> states);
        ResourceTransitionBarrier BarrierToState(WriteDepthStencilTextureResourceState state);
        ResourceTransitionBarrier BarrierToState(ReadWriteDepthStencilTextureResourceState state);*/
    };

    class ColorBufferResource : public Resource {
    public:
        using Resource::Resource;
        ColorBufferResource(
            const Device& device, 
            ResourceFormat::Color dataType,
            uint64_t width,
            HeapType heapType = HeapType::Default
        );
    };

    class TypelessBufferResource : public Resource {
    public:
        using Resource::Resource;
        TypelessBufferResource(
            const Device& device, 
            ResourceFormat::TypelessColor dataType,
            uint64_t width, 
            HeapType heapType = HeapType::Default
        );
    };

}

