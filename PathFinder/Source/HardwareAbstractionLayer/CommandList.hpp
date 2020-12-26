#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "GraphicAPIObject.hpp"
#include "Device.hpp"
#include "CommandAllocator.hpp"
#include "Viewport.hpp"
#include "PipelineState.hpp"
#include "Descriptor.hpp"
#include "Resource.hpp"
#include "ResourceBarrier.hpp"
#include "DescriptorHeap.hpp"
#include "Fence.hpp"
#include "Buffer.hpp"
#include "QueryHeap.hpp"
#include "RayTracingAccelerationStructure.hpp"
#include "ResourceFootprint.hpp"
#include "ShaderRegister.hpp"
#include "Types.hpp"

#include <Geometry/Rect2D.hpp>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <optional>
#include <array>

#include <aftermath/GFSDK_Aftermath.h>

namespace HAL
{
    class CommandList : public GraphicAPIObject
    {
    public:
        CommandList(const Device& device, CommandAllocator* allocator, D3D12_COMMAND_LIST_TYPE type);
        CommandList(CommandList&& that) = default;
        CommandList(const CommandList& that) = delete;
        CommandList& operator=(const CommandList& that) = delete;
        virtual ~CommandList() = 0; 

        void Reset();
        void Close();

        void ExtractQueryData(const QueryHeap& heap, uint64_t startIndex, uint64_t queryCount, const Buffer& readbackBuffer);
        void EndQuery(const QueryHeap& heap, uint64_t queryIndex);

        void SetDebugName(const std::string& name) override;

    protected:
        CommandAllocator* mCommandAllocator = nullptr;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> mList;
        bool mIsClosed = false;
        std::optional<GFSDK_Aftermath_ContextHandle> mAftermathHandle;

    public:
        inline ID3D12GraphicsCommandList* D3DList() const { return mList.Get(); }
        inline std::optional<GFSDK_Aftermath_ContextHandle> AftermathHandle() const { return mAftermathHandle; }
    };



    class CopyCommandListBase : public CommandList {
    public:
        using CommandList::CommandList;

        void InsertBarrier(const ResourceBarrier& barrier);
        void InsertBarriers(const ResourceBarrierCollection& collection);
        void CopyResource(const Resource& source, Resource& destination);

        void CopyBufferRegion(
            const Buffer& source, const Buffer& destination,
            uint64_t sourceOffset, uint64_t copyRegionSize, uint64_t destinationOffset);

        void CopyTextureRegion(
            const Texture& source, const Texture& destination,
            uint16_t sourceSubresource, uint16_t destinationSubresource,
            const glm::ivec3& sourceOrigin, const glm::ivec3& destinationOrigin,
            const Geometry::Dimensions& regionDimensions
        );

        void CopyBufferToTexture(const Buffer& buffer, const Texture& texture, const SubresourceFootprint& footprint);
        void CopyTextureToBuffer(const Texture& texture, const Buffer& buffer, const SubresourceFootprint& footprint);
    };



    class ComputeCommandListBase : public CopyCommandListBase {
    public:
        using CopyCommandListBase::CopyCommandListBase;

        void SetPipelineState(const RayTracingPipelineState& state);
        void SetPipelineState(const ComputePipelineState& state);
        void SetComputeRootSignature(const RootSignature& signature);

        template <class T> void SetComputeRootConstants(const T& constants, uint32_t rootParameterIndex);

        void SetComputeRootConstantBuffer(GPUAddress bufferAddress, uint32_t rootParameterIndex);
        void SetComputeRootConstantBuffer(const Buffer& cbResource, uint32_t rootParameterIndex);
        void SetComputeRootShaderResource(const Resource& resource, uint32_t rootParameterIndex);
        void SetComputeRootUnorderedAccessResource(const Resource& resource, uint32_t rootParameterIndex);
        void SetComputeRootDescriptorTable(DescriptorAddress tableStartAddress, uint32_t rootParameterIndex);
        void SetDescriptorHeap(const CBSRUADescriptorHeap& heap);
        void SetDescriptorHeaps(const CBSRUADescriptorHeap& cbsruaHeap, const SamplerDescriptorHeap& samplerHeap);
        void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
        void DispatchRays(const RayDispatchInfo& dispatchInfo);
    };



    class GraphicsCommandListBase : public ComputeCommandListBase {
    public:
        using ComputeCommandListBase::ComputeCommandListBase;
        using ComputeCommandListBase::SetPipelineState;

        void SetViewport(const Viewport& viewport);
        void SetScissor(const Geometry::Rect2D& scissorRect);
        void SetRenderTarget(const RTDescriptor& rtDescriptor, const DSDescriptor* depthStencilDescriptor = nullptr);
        void ClearRenderTarget(const RTDescriptor& rtDescriptor, const glm::vec4& color);
        void CleadDepthStencil(const DSDescriptor& dsDescriptor, float depthValue);
        void SetPrimitiveTopology(PrimitiveTopology topology);
        void SetPipelineState(const GraphicsPipelineState& state);
        void SetGraphicsRootSignature(const RootSignature& signature);

        template <size_t RTCount> void SetRenderTargets(const std::array<const RTDescriptor*, RTCount>& rtDescriptors, const DSDescriptor* depthStencilDescriptor = nullptr);
        template <class T> void SetGraphicsRootConstants(const T& constants, uint32_t rootParameterIndex);

        void SetGraphicsRootConstantBuffer(GPUAddress bufferAddress, uint32_t rootParameterIndex);
        void SetGraphicsRootConstantBuffer(const Buffer& cbResource, uint32_t rootParameterIndex);
        void SetGraphicsRootShaderResource(const Resource& resource, uint32_t rootParameterIndex);
        void SetGraphicsRootUnorderedAccessResource(const Resource& resource, uint32_t rootParameterIndex);
        void SetGraphicsRootDescriptorTable(DescriptorAddress tableStartAddress, uint32_t rootParameterIndex);
    };



    class CopyCommandList : public CopyCommandListBase {
    public:
        CopyCommandList(const Device& device, CopyCommandAllocator* commandAllocator);
        ~CopyCommandList() = default;
    };

    class ComputeCommandList : public ComputeCommandListBase {
    public:
        ComputeCommandList(const Device& device, ComputeCommandAllocator* commandAllocator);
        ~ComputeCommandList() = default;

        void BuildRaytracingAccelerationStructure(const RayTracingAccelerationStructure& as);
    };
    
    class BundleCommandList : public GraphicsCommandListBase {
    public:
        BundleCommandList(const Device& device, BundleCommandAllocator* commandAllocator);
        ~BundleCommandList() = default;
    };

    class GraphicsCommandList : public GraphicsCommandListBase {
    public:
        GraphicsCommandList(const Device& device, GraphicsCommandAllocator* commandAllocator);
        ~GraphicsCommandList() = default;

        void ExecuteBundle(const BundleCommandList& bundle);

        void BuildRaytracingAccelerationStructure(const RayTracingAccelerationStructure& as);

        void Draw(uint32_t vertexCount, uint32_t vertexStart);
        void DrawInstanced(uint32_t vertexCount, uint32_t vertexStart, uint32_t instanceCount);
        void DrawIndexed(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart);
        void DrawIndexedInstanced(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart, uint32_t instanceCount);
    };
}

#include "CommandList.inl"