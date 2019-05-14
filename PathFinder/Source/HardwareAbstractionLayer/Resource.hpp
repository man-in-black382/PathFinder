#pragma once

#include <wrl.h>
#include <dxgi.h>
#include <cstdint>
#include <d3d12.h>

#include "Device.hpp"
#include "ResourceFormat.hpp"
#include "ResourceState.hpp"
#include "ResourceBarrier.hpp"
#include "Utils.h"

#include "../Geometry/Dimensions.hpp"

namespace HAL
{
    template <typename ReadStateT, typename WriteStateT, typename ReadWriteStateT>
    class Resource
    {
    public:
        enum class HeapType { Default, Upload, Readback };

        Resource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr);
        virtual ~Resource() = 0;

        ResourceTransitionBarrier BarrierToStates(std::initializer_list<ReadStateT> states);
        ResourceTransitionBarrier BarrierToState(ReadStateT state);
        ResourceTransitionBarrier BarrierToState(WriteStateT state);
        ResourceTransitionBarrier BarrierToState(ReadWriteStateT state);

    protected:
        Resource(
            const Device& device,
            const ResourceFormat& format,
            HeapType heapType,
            ResourceStateBaseT initialState, 
            std::initializer_list<ResourceStateBaseT> expectedStates
        );

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
        D3D12_RESOURCE_DESC mDescription;
        D3D12_RESOURCE_STATES mInitialState;
        Geometry::Dimensions mDimensions;

        void ConvertInitialStatesToFlags();

    public:
        inline const auto D3DPtr() const { return mResource.Get(); }
        inline const auto& D3DDescription() const { return mDescription; };
        inline const auto& Dimensions() const { return mDimensions; };
    };

    template <typename ReadStateT, typename WriteStateT, typename ReadWriteStateT>
    Resource<ReadStateT, WriteStateT, ReadWriteStateT>::~Resource() {}

    template <typename ReadStateT, typename WriteStateT, typename ReadWriteStateT>
    Resource<ReadStateT, WriteStateT, ReadWriteStateT>::Resource(const Microsoft::WRL::ComPtr<ID3D12Resource>& existingResourcePtr)
        : mResource(existingResourcePtr), mDescription(mResource->GetDesc()) {}

    template <typename ReadStateT, typename WriteStateT, typename ReadWriteStateT>
    Resource<ReadStateT, WriteStateT, ReadWriteStateT>::Resource(
        const Device& device, 
        const ResourceFormat& format,
        HeapType heapType,
        ResourceStateBaseT initialState,
        std::initializer_list<ResourceStateBaseT> expectedStates)
        : 
        mInitialState(initialState.D3DState()), mDescription(format.D3DResourceDescription())
    {
        D3D12_HEAP_PROPERTIES heapProperties{};

        switch (heapType) {
        case HeapType::Default:
            heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
            break;
        case HeapType::Upload:
            heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
            mInitialState |= GenericReadResourceState.D3DState();
            break;
        case HeapType::Readback:
            heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
            break;
        }
        
        D3D12_CLEAR_VALUE clearValue;

        ThrowIfFailed(device.D3DPtr()->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            mDescription,
            mInitialState,
            nullptr,
            IID_PPV_ARGS(&mResource)
        ));
    }

    template <typename ReadStateT, typename WriteStateT, typename ReadWriteStateT>
    void Resource<ReadStateT, WriteStateT, ReadWriteStateT>::ConvertInitialStatesToFlags()
    {
        /* D3D12_RESOURCE_FLAG_NONE = 0,
             D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET = 0x1,
             D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL = 0x2,
             D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS = 0x4,
             D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE = 0x8,
             D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER = 0x10,
             D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS = 0x20,
             D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY = 0x40*/
        if (initial)
    }

    template <typename ReadStateT, typename WriteStateT, typename ReadWriteStateT>
    ResourceTransitionBarrier Resource<ReadStateT, WriteStateT, ReadWriteStateT>::BarrierToStates(std::initializer_list<ReadStateT> states)
    {
        return ResourceTransitionBarrier(mInitialState, D3DResourceStatesFromVariantList(states), mResource);
    }

    template <typename ReadStateT, typename WriteStateT, typename ReadWriteStateT>
    ResourceTransitionBarrier Resource<ReadStateT, WriteStateT, ReadWriteStateT>::BarrierToState(ReadWriteStateT state)
    {
        return ResourceTransitionBarrier(mInitialState, D3DResourceStatesFromVariant(state), mResource);
    }

    template <typename ReadStateT, typename WriteStateT, typename ReadWriteStateT>
    ResourceTransitionBarrier Resource<ReadStateT, WriteStateT, ReadWriteStateT>::BarrierToState(WriteStateT state)
    {
        return ResourceTransitionBarrier(mInitialState, D3DResourceStatesFromVariant(state), mResource);
    }

    template <typename ReadStateT, typename WriteStateT, typename ReadWriteStateT>
    ResourceTransitionBarrier Resource<ReadStateT, WriteStateT, ReadWriteStateT>::BarrierToState(ReadStateT state)
    {
        return ResourceTransitionBarrier(mInitialState, D3DResourceStatesFromVariant(state), mResource);
    }



    class ColorTextureResource 
        : public Resource<ReadTextureResourceStateT, WriteTextureResourceStateT, ReadWriteTextureResourceStateT> 
    {
    public:
        using Resource::Resource;
        ColorTextureResource(
            const Device& device, 
            ResourceFormat::Color dataType, 
            ResourceFormat::TextureKind kind, 
            const Geometry::Dimensions& dimensions, 
            HeapType heapType = HeapType::Default
        );
    };

    class TypelessTextureResource 
        : public Resource<ReadTextureResourceStateT, WriteTextureResourceStateT, ReadWriteTextureResourceStateT>
    {
    public:
        using Resource::Resource;
        TypelessTextureResource(
            const Device& device,
            ResourceFormat::TypelessColor dataType,
            ResourceFormat::TextureKind kind, 
            const Geometry::Dimensions& dimensions,
            HeapType heapType = HeapType::Default
        );
    };

    class DepthStencilTextureResource 
        : public Resource<ReadDepthStencilTextureResourceStateT, WriteDepthStencilTextureResourceStateT, ReadWriteDepthStencilTextureResourceStateT> 
    {
    public:
        using Resource::Resource;
        DepthStencilTextureResource(
            const Device& device, 
            ResourceFormat::DepthStencil dataType, 
            const Geometry::Dimensions& dimensions,
            HeapType heapType = HeapType::Default
        );
    };

    class ColorBufferResource 
        : public Resource<ReadBufferResourceStateT, WriteBufferResourceStateT, ReadWriteBufferResourceStateT>
    {
    public:
        using Resource::Resource;
        ColorBufferResource(
            const Device& device, 
            ResourceFormat::Color dataType,
            uint64_t width,
            HeapType heapType = HeapType::Default
        );
    };

    /* class TypelessBufferResource : public Resource {
     public:
         using Resource::Resource;
         TypelessBufferResource(
             const Device& device,
             ResourceFormat::TypelessColor dataType,
             uint64_t width,
             HeapType heapType = HeapType::Default
         );
     };*/

}

