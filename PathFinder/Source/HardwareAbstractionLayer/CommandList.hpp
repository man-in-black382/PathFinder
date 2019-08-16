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
        inline ID3D12GraphicsCommandList* D3DList() const { return mList.Get(); }
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

        void SetPipelineState(const ComputePipelineState& state);
        void SetComputeRootSignature(const RootSignature& signature);

        template <class... Descriptors> void SetDescriptorHeap(const DescriptorHeap<Descriptors...>& heap);
        template <class T> void SetComputeRootConstantBuffer(const BufferResource<T>& cbResource, uint32_t rootParameterIndex);

        void SetComputeRootShaderResource(const Resource& resource, uint32_t rootParameterIndex);
        void SetComputeRootUnorderedAccessResource(const Resource& resource, uint32_t rootParameterIndex);
        void SetComputeRootDescriptorTable(const GPUDescriptor& baseDescriptor, uint32_t rootParameterIndex);
    };

    template <class... Descriptors>
    void ComputeCommandListBase::SetDescriptorHeap(const DescriptorHeap<Descriptors...>& heap)
    {
        auto ptr = heap.D3DHeap();
        mList->SetDescriptorHeaps(1, (ID3D12DescriptorHeap* const*)&ptr);
    }

    template <class T>
    void ComputeCommandListBase::SetComputeRootConstantBuffer(const BufferResource<T>& cbResource, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootConstantBufferView(rootParameterIndex, cbResource.GPUVirtualAddress());
    }



    class GraphicsCommandListBase : public ComputeCommandListBase {
    public:
        using ComputeCommandListBase::ComputeCommandListBase;

        void SetViewport(const Viewport& viewport);
        void SetRenderTarget(const RTDescriptor& rtDescriptor, std::optional<const DSDescriptor> depthStencilDescriptor = std::nullopt);
        void ClearRenderTarget(const RTDescriptor& rtDescriptor, const Foundation::Color& color);
        void CleadDepthStencil(const DSDescriptor& dsDescriptor, float depthValue);
        void SetFence(const Fence& fence);
        void SetVertexBuffer(const VertexBufferDescriptor& descriptor);
        void SetIndexBuffer(const IndexBufferDescriptor& descriptor);
        void SetPrimitiveTopology(PrimitiveTopology topology);
        void SetPipelineState(const PipelineState& state);
        void SetGraphicsRootSignature(const RootSignature& signature);

        template <class T> void SetGraphicsRootConstantBuffer(const BufferResource<T>& cbResource, uint32_t rootParameterIndex);

        void SetGraphicsRootShaderResource(const Resource& resource, uint32_t rootParameterIndex);
        void SetGraphicsRootUnorderedAccessResource(const Resource& resource, uint32_t rootParameterIndex);
        void SetGraphicsRootDescriptorTable(const GPUDescriptor& baseDescriptor, uint32_t rootParameterIndex);
    };

    template <class T>
    void GraphicsCommandListBase::SetGraphicsRootConstantBuffer(const BufferResource<T>& cbResource, uint32_t rootParameterIndex)
    {
        mList->SetGraphicsRootConstantBufferView(rootParameterIndex, cbResource.GPUVirtualAddress());
    }



    class CopyCommandList : public CopyCommandListBase {
    public:
        CopyCommandList(const Device& device, const CopyCommandAllocator& allocator);
        ~CopyCommandList() = default;
    };

    class ComputeCommandList : public ComputeCommandListBase {
    public:
        ComputeCommandList(const Device& device, const ComputeCommandAllocator& allocator);
        ~ComputeCommandList() = default;
    };
    
    class BundleCommandList : public GraphicsCommandListBase {
    public:
        BundleCommandList(const Device& device, const BundleCommandAllocator& allocator);
        ~BundleCommandList() = default;
    };

    class GraphicsCommandList : public GraphicsCommandListBase {
    public:
        GraphicsCommandList(const Device& device, const DirectCommandAllocator& allocator);
        ~GraphicsCommandList() = default;

        void ExecuteBundle(const BundleCommandList& bundle);

        void Draw(uint32_t vertexCount, uint32_t vertexStart);
        void DrawInstanced(uint32_t vertexCount, uint32_t vertexStart, uint32_t instanceCount);
        void DrawIndexed(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart);
        void DrawIndexedInstanced(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart, uint32_t instanceCount);
    };
}

