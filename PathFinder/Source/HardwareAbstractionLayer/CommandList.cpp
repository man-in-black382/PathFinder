#include "CommandList.hpp"
#include "Utils.h"

namespace HAL
{

    CommandList::CommandList(const Device& device, const CommandAllocator& allocator, D3D12_COMMAND_LIST_TYPE type)
    {
        ThrowIfFailed(device.D3DDevice()->CreateCommandList(0, type, allocator.D3DPtr(), nullptr, IID_PPV_ARGS(&mList)));
    }

    CommandList::~CommandList() {}

    void CommandList::Reset(const CommandAllocator& allocator)
    {
       ThrowIfFailed(mList->Reset(allocator.D3DPtr(), nullptr));
    }

    void CommandList::Close()
    {
       ThrowIfFailed(mList->Close());
    }



    void CopyCommandListBase::InsertBarrier(const ResourceBarrier& barrier)
    {
        mList->ResourceBarrier(1, &barrier.D3DBarrier());
    }

    void CopyCommandListBase::InsertBarriers(const ResourceBarrierCollection& collection)
    {
        mList->ResourceBarrier((UINT)collection.BarrierCount(), collection.D3DBarriers());
    }

    void CopyCommandListBase::CopyResource(const Resource& source, Resource& destination)
    {
        mList->CopyResource(destination.D3DPtr(), source.D3DPtr());
    }

    void CopyCommandListBase::CopyTextureRegion(
        const TextureResource& source, TextureResource& destination,
        uint16_t sourceSubresource, uint16_t destinationSubresource, 
        const glm::ivec3& sourceOrigin, const glm::ivec3& destinationOrigin,
        const Geometry::Dimensions& regionDimensions)
    {
        D3D12_TEXTURE_COPY_LOCATION srcLocation{};
        D3D12_TEXTURE_COPY_LOCATION dstLocation{};

        srcLocation.pResource = source.D3DPtr();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLocation.SubresourceIndex = sourceSubresource;

        dstLocation.pResource = destination.D3DPtr();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = destinationSubresource;

        D3D12_BOX srcBox{};
        //srcBox.

        //mList->CopyTextureRegion(&dstLocation, destinationRectOrigin.x, destinationRectOrigin.y, destinationRectOrigin.z, &srcLocation, )
    }

    void ComputeCommandListBase::SetComputeRootShaderResource(const Resource& resource, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootShaderResourceView(rootParameterIndex, resource.GPUVirtualAddress());
    }

    void ComputeCommandListBase::SetComputeRootUnorderedAccessResource(const Resource& resource, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootUnorderedAccessView(rootParameterIndex, resource.GPUVirtualAddress());
    }

    void ComputeCommandListBase::SetComputeRootDescriptorTable(const GPUDescriptor& baseDescriptor, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootDescriptorTable(rootParameterIndex, baseDescriptor.GPUHandle());
    }

    void ComputeCommandListBase::SetDescriptorHeap(const CBSRUADescriptorHeap& heap)
    {
        auto ptr = heap.D3DHeap();
        mList->SetDescriptorHeaps(1, &ptr);
    }

    void ComputeCommandListBase::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        mList->Dispatch(groupCountX, groupCountY, groupCountZ);
    }

    void ComputeCommandListBase::SetPipelineState(const ComputePipelineState& state)
    {
        mList->SetPipelineState(state.D3DCompiledState());
    }

    void ComputeCommandListBase::SetComputeRootSignature(const RootSignature& signature)
    {
        mList->SetComputeRootSignature(signature.D3DSignature());
    }



    void GraphicsCommandListBase::SetViewport(const Viewport& viewport)
    {
        auto d3dViewport = viewport.D3DViewport();
        mList->RSSetViewports(1, &d3dViewport);
        D3D12_RECT scissorRect{ viewport.X, viewport.Y, viewport.Width, viewport.Height };
        mList->RSSetScissorRects(1, &scissorRect);
    }

    void GraphicsCommandListBase::SetRenderTarget(const RTDescriptor& rtDescriptor, std::optional<const DSDescriptor> depthStencilDescriptor)
    {
        const D3D12_CPU_DESCRIPTOR_HANDLE* dsHandle = depthStencilDescriptor.has_value() ? &depthStencilDescriptor->CPUHandle() : nullptr;
        mList->OMSetRenderTargets(1, &rtDescriptor.CPUHandle(), false, dsHandle);
    }

    void GraphicsCommandListBase::ClearRenderTarget(const RTDescriptor& rtDescriptor, const Foundation::Color& color)
    {
        mList->ClearRenderTargetView(rtDescriptor.CPUHandle(), color.Ptr(), 0, nullptr);
    }

    void GraphicsCommandListBase::CleadDepthStencil(const DSDescriptor& dsDescriptor, float depthValue)
    {
        mList->ClearDepthStencilView(dsDescriptor.CPUHandle(), D3D12_CLEAR_FLAG_DEPTH, depthValue, 0, 0, nullptr);
    }

    void GraphicsCommandListBase::SetFence(const Fence& fence)
    {
        
    }

    void GraphicsCommandListBase::SetVertexBuffer(const VertexBufferDescriptor& descriptor)
    {
        mList->IASetVertexBuffers(0, 1, &descriptor.D3DDescriptor());
    }
   
    void GraphicsCommandListBase::SetIndexBuffer(const IndexBufferDescriptor& descriptor)
    {
        mList->IASetIndexBuffer(&descriptor.D3DDescriptor());
    }

    void GraphicsCommandListBase::SetPrimitiveTopology(PrimitiveTopology topology)
    {
        mList->IASetPrimitiveTopology(D3DPrimitiveTopology(topology));
    }

    void GraphicsCommandListBase::SetPipelineState(const PipelineState& state)
    {
        mList->SetPipelineState(state.D3DCompiledState());
    }

    void GraphicsCommandListBase::SetGraphicsRootSignature(const RootSignature& signature)
    {
        mList->SetGraphicsRootSignature(signature.D3DSignature());
    }
    
    void GraphicsCommandListBase::SetGraphicsRootShaderResource(const Resource& resource, uint32_t rootParameterIndex)
    {
        mList->SetGraphicsRootShaderResourceView(rootParameterIndex, resource.GPUVirtualAddress());
    }

    void GraphicsCommandListBase::SetGraphicsRootUnorderedAccessResource(const Resource& resource, uint32_t rootParameterIndex)
    {
        mList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, resource.GPUVirtualAddress());
    }

    void GraphicsCommandListBase::SetGraphicsRootDescriptorTable(const GPUDescriptor& baseDescriptor, uint32_t rootParameterIndex)
    {
        mList->SetGraphicsRootDescriptorTable(rootParameterIndex, baseDescriptor.GPUHandle());
    }



    CopyCommandList::CopyCommandList(const Device& device, const CopyCommandAllocator& allocator)
        : CopyCommandListBase(device, allocator, D3D12_COMMAND_LIST_TYPE_COPY) {}



    ComputeCommandList::ComputeCommandList(const Device& device, const ComputeCommandAllocator& allocator)
        : ComputeCommandListBase(device, allocator, D3D12_COMMAND_LIST_TYPE_COMPUTE) {}

    void ComputeCommandList::BuildRaytracingAccelerationStructure(const RayTracingAccelerationStructure& as)
    {
        mList->BuildRaytracingAccelerationStructure(&as.D3DAccelerationStructure(), 0, nullptr);
    }



    BundleCommandList::BundleCommandList(const Device& device, const BundleCommandAllocator& allocator)
        : GraphicsCommandListBase(device, allocator, D3D12_COMMAND_LIST_TYPE_BUNDLE) {}



    GraphicsCommandList::GraphicsCommandList(const Device& device, const GraphicsCommandAllocator& allocator)
        : GraphicsCommandListBase(device, allocator, D3D12_COMMAND_LIST_TYPE_DIRECT) {}

    void GraphicsCommandList::ExecuteBundle(const BundleCommandList& bundle)
    {

    }

    void GraphicsCommandList::BuildRaytracingAccelerationStructure(const RayTracingAccelerationStructure& as)
    {
        mList->BuildRaytracingAccelerationStructure(&as.D3DAccelerationStructure(), 0, nullptr);
    }

    void GraphicsCommandList::Draw(uint32_t vertexCount, uint32_t vertexStart)
    {
        mList->DrawInstanced(vertexCount, 1, vertexStart, 0);
    }

    void GraphicsCommandList::DrawInstanced(uint32_t vertexCount, uint32_t vertexStart, uint32_t instanceCount)
    {
        mList->DrawInstanced(vertexCount, instanceCount, vertexStart, 1);
    }

    void GraphicsCommandList::DrawIndexed(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart)
    {
        mList->DrawIndexedInstanced(indexCount, 1, indexStart, vertexStart, 0);
    }

    void GraphicsCommandList::DrawIndexedInstanced(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart, uint32_t instanceCount)
    {
        mList->DrawIndexedInstanced(indexCount, instanceCount, indexStart, vertexStart, 1);
    }

}