#include "CommandList.hpp"
#include "Utils.h"

#include <aftermath/AftermathHelpers.hpp>

namespace HAL
{

    CommandList::CommandList(const Device& device, CommandAllocator* allocator, D3D12_COMMAND_LIST_TYPE type)
        : mCommandAllocator{ allocator }
    {
        ThrowIfFailed(device.D3DDevice()->CreateCommandList(0, type, mCommandAllocator->D3DPtr(), nullptr, IID_PPV_ARGS(&mList)));

        if (device.AftermathEnabled())
        {
            mAftermathHandle = GFSDK_Aftermath_ContextHandle{};
            AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_DX12_CreateContextHandle(mList.Get(), &mAftermathHandle.value()));
        }
    }

    CommandList::~CommandList()
    {
        if (mAftermathHandle)
        {
            GFSDK_Aftermath_ReleaseContextHandle(*mAftermathHandle);
        }
    }

    void CommandList::Reset()
    {
        ThrowIfFailed(mList->Reset(mCommandAllocator->D3DPtr(), nullptr));
        mIsClosed = false;
    }

    void CommandList::Close()
    {
        if (!mIsClosed)
        {
            ThrowIfFailed(mList->Close());
            mIsClosed = true;
        }
    }

    void CommandList::ExtractQueryData(const QueryHeap& heap, uint64_t startIndex, uint64_t queryCount, const Buffer& readbackBuffer)
    {
        mList->ResolveQueryData(heap.D3DQueryHeap(), heap.D3DQueryType(), startIndex, queryCount, readbackBuffer.D3DResource(), 0);
    }

    void CommandList::EndQuery(const QueryHeap& heap, uint64_t queryIndex)
    {
        mList->EndQuery(heap.D3DQueryHeap(), heap.D3DQueryType(), queryIndex);
    }

    void CommandList::SetDebugName(const std::string& name)
    {
        mList->SetName(StringToWString(name).c_str());
    }



    void CopyCommandListBase::InsertBarrier(const ResourceBarrier& barrier)
    {
        mList->ResourceBarrier(1, &barrier.D3DBarrier());
    }

    void CopyCommandListBase::InsertBarriers(const ResourceBarrierCollection& collection)
    {
        if (collection.BarrierCount() == 0) return;

        mList->ResourceBarrier((UINT)collection.BarrierCount(), collection.D3DBarriers());
    }

    void CopyCommandListBase::CopyResource(const Resource& source, Resource& destination)
    {
        mList->CopyResource(destination.D3DResource(), source.D3DResource());
    }

    void CopyCommandListBase::CopyTextureRegion(
        const Texture& source, const Texture& destination,
        uint16_t sourceSubresource, uint16_t destinationSubresource, 
        const glm::ivec3& sourceOrigin, const glm::ivec3& destinationOrigin,
        const Geometry::Dimensions& regionDimensions)
    {
        D3D12_TEXTURE_COPY_LOCATION srcLocation{};
        D3D12_TEXTURE_COPY_LOCATION dstLocation{};

        srcLocation.pResource = source.D3DResource();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLocation.SubresourceIndex = sourceSubresource;

        dstLocation.pResource = destination.D3DResource();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = destinationSubresource;

        D3D12_BOX srcBox{};
        //srcBox.

        //mList->CopyTextureRegion(&dstLocation, destinationRectOrigin.x, destinationRectOrigin.y, destinationRectOrigin.z, &srcLocation, )
    }

    void CopyCommandListBase::CopyBufferRegion(
        const Buffer& source, const Buffer& destination,
        uint64_t sourceOffset, uint64_t copyRegionSize, uint64_t destinationOffset)
    {
        mList->CopyBufferRegion(destination.D3DResource(), destinationOffset, source.D3DResource(), sourceOffset, copyRegionSize);
    }

    void CopyCommandListBase::CopyBufferToTexture(const Buffer& buffer, const Texture& texture, const SubresourceFootprint& footprint)
    {
        D3D12_TEXTURE_COPY_LOCATION srcLocation{};
        D3D12_TEXTURE_COPY_LOCATION dstLocation{};

        srcLocation.pResource = buffer.D3DResource();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.PlacedFootprint = footprint.D3DFootprint();

        dstLocation.pResource = texture.D3DResource();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = footprint.IndexInResource();

        mList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    }

    void CopyCommandListBase::CopyTextureToBuffer(const Texture& texture, const Buffer& buffer, const SubresourceFootprint& footprint)
    {
        D3D12_TEXTURE_COPY_LOCATION srcLocation{};
        D3D12_TEXTURE_COPY_LOCATION dstLocation{};

        srcLocation.pResource = texture.D3DResource();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLocation.SubresourceIndex = footprint.IndexInResource();

        dstLocation.pResource = buffer.D3DResource();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dstLocation.PlacedFootprint = footprint.D3DFootprint();

        mList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
    }



    void ComputeCommandListBase::SetComputeRootConstantBuffer(GPUAddress bufferAddress, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootConstantBufferView(rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS{ bufferAddress });
    }

    void ComputeCommandListBase::SetComputeRootConstantBuffer(const Buffer& cbResource, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootConstantBufferView(rootParameterIndex, cbResource.GPUVirtualAddress());
    }

    void ComputeCommandListBase::SetComputeRootShaderResource(const Resource& resource, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootShaderResourceView(rootParameterIndex, resource.GPUVirtualAddress());
    }

    void ComputeCommandListBase::SetComputeRootUnorderedAccessResource(const Resource& resource, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootUnorderedAccessView(rootParameterIndex, resource.GPUVirtualAddress());
    }

    void ComputeCommandListBase::SetComputeRootDescriptorTable(DescriptorAddress tableStartAddress, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ tableStartAddress });
    }

    void ComputeCommandListBase::SetDescriptorHeap(const CBSRUADescriptorHeap& heap)
    {
        auto ptr = heap.D3DHeap();
        mList->SetDescriptorHeaps(1, &ptr);
    }

    void ComputeCommandListBase::SetDescriptorHeaps(const CBSRUADescriptorHeap& cbsruaHeap, const SamplerDescriptorHeap& samplerHeap)
    {
        std::array<ID3D12DescriptorHeap*, 2> heaps{ cbsruaHeap.D3DHeap(), samplerHeap.D3DHeap() };
        ID3D12DescriptorHeap* const* ppDescriptorHeaps = heaps.data();
        mList->SetDescriptorHeaps(2, ppDescriptorHeaps);
    }

    void ComputeCommandListBase::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        mList->Dispatch(groupCountX, groupCountY, groupCountZ);
    }

    void ComputeCommandListBase::DispatchRays(const RayDispatchInfo& dispatchInfo)
    {
        mList->DispatchRays(&dispatchInfo.D3DDispatchInfo());
    }

    void ComputeCommandListBase::SetPipelineState(const ComputePipelineState& state)
    {
        mList->SetPipelineState(state.D3DCompiledState());
    }

    void ComputeCommandListBase::SetPipelineState(const RayTracingPipelineState& state)
    {
        mList->SetPipelineState1(state.D3DCompiledState());
    }

    void ComputeCommandListBase::SetComputeRootSignature(const RootSignature& signature)
    {
        mList->SetComputeRootSignature(signature.D3DSignature());
    }



    void GraphicsCommandListBase::SetViewport(const Viewport& viewport)
    {
        auto d3dViewport = viewport.D3DViewport();
        mList->RSSetViewports(1, &d3dViewport);
    }

    void GraphicsCommandListBase::SetScissor(const Geometry::Rect2D& scissorRect)
    {
        D3D12_RECT d3dRect{ scissorRect.Origin.x, scissorRect.Origin.y, scissorRect.Size.Width, scissorRect.Size.Height };
        mList->RSSetScissorRects(1, &d3dRect);
    }

    void GraphicsCommandListBase::SetRenderTarget(const RTDescriptor& rtDescriptor, const DSDescriptor* depthStencilDescriptor)
    {
        const D3D12_CPU_DESCRIPTOR_HANDLE* dsHandle = depthStencilDescriptor ? &depthStencilDescriptor->CPUHandle() : nullptr;
        mList->OMSetRenderTargets(1, &rtDescriptor.CPUHandle(), false, dsHandle);
    }

    void GraphicsCommandListBase::ClearRenderTarget(const RTDescriptor& rtDescriptor, const glm::vec4& color)
    {
        mList->ClearRenderTargetView(rtDescriptor.CPUHandle(), (float*)&color, 0, nullptr);
    }

    void GraphicsCommandListBase::CleadDepthStencil(const DSDescriptor& dsDescriptor, float depthValue)
    {
        mList->ClearDepthStencilView(dsDescriptor.CPUHandle(), D3D12_CLEAR_FLAG_DEPTH, depthValue, 0, 0, nullptr);
    }

    void GraphicsCommandListBase::SetPrimitiveTopology(PrimitiveTopology topology)
    {
        mList->IASetPrimitiveTopology(D3DPrimitiveTopology(topology));
    }

    void GraphicsCommandListBase::SetPipelineState(const GraphicsPipelineState& state)
    {
        mList->SetPipelineState(state.D3DCompiledState());
    }

    void GraphicsCommandListBase::SetGraphicsRootSignature(const RootSignature& signature)
    {
        mList->SetGraphicsRootSignature(signature.D3DSignature());
    }

    void GraphicsCommandListBase::SetGraphicsRootConstantBuffer(GPUAddress bufferAddress, uint32_t rootParameterIndex)
    {
        mList->SetGraphicsRootConstantBufferView(rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS{ bufferAddress });
    }

    void GraphicsCommandListBase::SetGraphicsRootConstantBuffer(const Buffer& cbResource, uint32_t rootParameterIndex)
    {
        mList->SetGraphicsRootConstantBufferView(rootParameterIndex, cbResource.GPUVirtualAddress());
    }

    void GraphicsCommandListBase::SetGraphicsRootShaderResource(const Resource& resource, uint32_t rootParameterIndex)
    {
        mList->SetGraphicsRootShaderResourceView(rootParameterIndex, resource.GPUVirtualAddress());
    }

    void GraphicsCommandListBase::SetGraphicsRootUnorderedAccessResource(const Resource& resource, uint32_t rootParameterIndex)
    {
        mList->SetGraphicsRootUnorderedAccessView(rootParameterIndex, resource.GPUVirtualAddress());
    }

    void GraphicsCommandListBase::SetGraphicsRootDescriptorTable(DescriptorAddress tableStartAddress, uint32_t rootParameterIndex)
    {
        mList->SetGraphicsRootDescriptorTable(rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE{ tableStartAddress });
    }



    CopyCommandList::CopyCommandList(const Device& device, CopyCommandAllocator* commandAllocator)
        : CopyCommandListBase(device, commandAllocator, D3D12_COMMAND_LIST_TYPE_COPY) {}



    ComputeCommandList::ComputeCommandList(const Device& device, ComputeCommandAllocator* commandAllocator)
        : ComputeCommandListBase(device, commandAllocator, D3D12_COMMAND_LIST_TYPE_COMPUTE) {}

    void ComputeCommandList::BuildRaytracingAccelerationStructure(const RayTracingAccelerationStructure& as)
    {
        mList->BuildRaytracingAccelerationStructure(&as.D3DAccelerationStructure(), 0, nullptr);
    }



    BundleCommandList::BundleCommandList(const Device& device, BundleCommandAllocator* commandAllocator)
        : GraphicsCommandListBase(device, commandAllocator, D3D12_COMMAND_LIST_TYPE_BUNDLE) {}



    GraphicsCommandList::GraphicsCommandList(const Device& device, GraphicsCommandAllocator* commandAllocator)
        : GraphicsCommandListBase(device, commandAllocator, D3D12_COMMAND_LIST_TYPE_DIRECT) {}

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