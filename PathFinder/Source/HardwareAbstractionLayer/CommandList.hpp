#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "Device.hpp"
#include "CommandAllocator.hpp"
#include "Viewport.hpp"
#include "PipelineState.hpp"
#include "Descriptor.hpp"
#include "Resource.hpp"
#include "ResourceBarrier.hpp"
#include "DescriptorHeap.hpp"
#include "Fence.hpp"
#include "BufferResource.hpp"

#include "../Foundation/Color.hpp"
#include "../Geometry/Rect2D.hpp"

#include <glm/vec3.hpp>

#include <optional>

namespace HAL
{
    class CommandList
    {
    public:
        CommandList(const Device& device, const CommandAllocator& allocator, D3D12_COMMAND_LIST_TYPE type);
        virtual ~CommandList() = 0; 

        void Reset(const CommandAllocator& allocator);
        void Close();

    protected:
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mList;

    public:
        inline const auto D3DList() const { return mList.Get(); }
    };



    class CopyCommandListBase : public CommandList {
    public:
        using CommandList::CommandList;

        void TransitionResourceState(const ResourceTransitionBarrier& barrier);
        void CopyResource(const Resource& source, Resource& destination);

        void CopyTextureRegion(
            const TextureResource& source, TextureResource& destination,
            uint16_t sourceSubresource, uint16_t destinationSubresource,
            const glm::ivec3& sourceOrigin, const glm::ivec3& destinationOrigin,
            const Geometry::Dimensions& regionDimensions
        );

        template <class T> 
        void CopyBufferRegion(
            const BufferResource<T>& source, BufferResource<T>& destination,
            uint64_t sourceOffset, uint64_t objectCount, uint64_t destinationOffset)
        {
            if (source.PaddedElementSize() != destination.PaddedElementSize()) {
                throw std::runtime_error("Buffers are misaligned. Copy will lay out data incorrectly.");
            }

            auto sourceOffsetInBytes = source.PaddedElementSize() * sourceOffset;
            auto destinationOffsetInBytes = destination.PaddedElementSize() * destinationOffset;
            auto regionSizeInBytes = source.PaddedElementSize() * objectCount;

            mList->CopyBufferRegion(destination.D3DPtr(), destinationOffsetInBytes, source.D3DPtr(), sourceOffsetInBytes, regionSizeInBytes);
        }
    };

    class ComputeCommandListBase : public CopyCommandListBase {
    public:
        using CopyCommandListBase::CopyCommandListBase;

        //void SetComputeRootConstantBuffer(const TypelessBufferResource& cbResource, uint32_t rootParameterIndex);
        //void SetComputeRootConstantBuffer(const ColorBufferResource& cbResource, uint32_t rootParameterIndex);
        void SetComputeRootShaderResource(const TypelessTextureResource& resource, uint32_t rootParameterIndex);
        void SetComputeRootShaderResource(const ColorTextureResource& resource, uint32_t rootParameterIndex);
        void SetComputeRootShaderResource(const DepthStencilTextureResource& resource, uint32_t rootParameterIndex);
        void SetComputeRootUnorderedAccessResource(const TypelessTextureResource& resource, uint32_t rootParameterIndex);
        void SetComputeRootUnorderedAccessResource(const ColorTextureResource& resource, uint32_t rootParameterIndex);
        void SetComputeRootUnorderedAccessResource(const DepthStencilTextureResource& resource, uint32_t rootParameterIndex);

        void SetDescriptorHeap(const DescriptorHeap& heap);
    };

    class DirectCommandListBase : public ComputeCommandListBase {
    public:
        using ComputeCommandListBase::ComputeCommandListBase;

        void SetViewport(const Viewport& viewport);
        void SetRenderTarget(const RTDescriptor& rtDescriptor, std::optional<const DSDescriptor> depthStencilDescriptor = std::nullopt);
        void ClearRenderTarget(const RTDescriptor& rtDescriptor, const Foundation::Color& color);
        void CleadDepthStencil(const DSDescriptor& dsDescriptor, float depthValue);
        void SetFence(const Fence& fence);
        void SetVertexBuffer(const VertexBufferDescriptor& descriptor);
        void SetPrimitiveTopology(PrimitiveTopology topology);
    };



    class CopyCommandList : public CopyCommandListBase {
    public:
        CopyCommandList(Device& device, CopyCommandAllocator& allocator);
        ~CopyCommandList() = default;
    };

    class ComputeCommandList : public ComputeCommandListBase {
    public:
        ComputeCommandList(Device& device, ComputeCommandAllocator& allocator);
        ~ComputeCommandList() = default;

        void SetPipelineState(const ComputePipelineState& state);
    };
    
    class BundleCommandList : public DirectCommandListBase {
    public:
        BundleCommandList(Device& device, BundleCommandAllocator& allocator);
        ~BundleCommandList() = default;
    };

    class DirectCommandList : public DirectCommandListBase {
    public:
        DirectCommandList(const Device& device, const DirectCommandAllocator& allocator);
        ~DirectCommandList() = default;

        void ExecuteBundle(const BundleCommandList& bundle);
        void SetPipelineState(const GraphicsPipelineState& state);

        void Draw(uint32_t vertexCount, uint32_t vertexStart);
        void DrawInstanced(uint32_t vertexCount, uint32_t vertexStart, uint32_t instanceCount);
        void DrawIndexed(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart);
        void DrawIndexedInstanced(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart, uint32_t instanceCount);
    };
}

