#include "CommandList.hpp"
#include "Utils.h"

namespace HAL
{

    CommandList::CommandList(const Device& device, const CommandAllocator& allocator, D3D12_COMMAND_LIST_TYPE type)
    {
        ThrowIfFailed(device.D3DPtr()->CreateCommandList(0, type, allocator.D3DPtr(), nullptr, IID_PPV_ARGS(&mList)));
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

    /* void ComputeCommandListBase::SetComputeRootConstantBuffer(const TypelessBufferResource& cbResource, uint32_t rootParameterIndex)
    { 
        mList->SetComputeRootConstantBufferView(rootParameterIndex, cbResource.D3DPtr()->GetGPUVirtualAddress());
    }

    void ComputeCommandListBase::SetComputeRootConstantBuffer(const ColorBufferResource& cbResource, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootConstantBufferView(rootParameterIndex, cbResource.D3DPtr()->GetGPUVirtualAddress());
    }*/

    void ComputeCommandListBase::SetComputeRootShaderResource(const TypelessTextureResource& resource, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootShaderResourceView(rootParameterIndex, resource.D3DPtr()->GetGPUVirtualAddress());
    }

    void ComputeCommandListBase::SetComputeRootShaderResource(const ColorTextureResource& resource, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootShaderResourceView(rootParameterIndex, resource.D3DPtr()->GetGPUVirtualAddress());
    }

    void ComputeCommandListBase::SetComputeRootShaderResource(const DepthStencilTextureResource& resource, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootShaderResourceView(rootParameterIndex, resource.D3DPtr()->GetGPUVirtualAddress());
    }

    void ComputeCommandListBase::SetComputeRootUnorderedAccessResource(const TypelessTextureResource& resource, uint32_t rootParameterIndex)
    {
        mList->SetComputeRootUnorderedAccessView(rootParameterIndex, resource.D3DPtr()->GetGPUVirtualAddress());
    }

    void ComputeCommandListBase::SetComputeRootUnorderedAccessResource(const ColorTextureResource& resource, uint32_t rootParameterIndex) 
    {
        mList->SetComputeRootUnorderedAccessView(rootParameterIndex, resource.D3DPtr()->GetGPUVirtualAddress());
    }

    void ComputeCommandListBase::SetComputeRootUnorderedAccessResource(const DepthStencilTextureResource& resource, uint32_t rootParameterIndex) 
    { 
        mList->SetComputeRootUnorderedAccessView(rootParameterIndex, resource.D3DPtr()->GetGPUVirtualAddress());
    }

    void ComputeCommandListBase::SetDescriptorHeap(const DescriptorHeap& heap)
    {
        auto ptr = heap.D3DPtr();
        mList->SetDescriptorHeaps(1, (ID3D12DescriptorHeap* const*)&ptr);
    }



    void DirectCommandListBase::SetViewport(const Viewport& viewport)
    {
        auto d3dViewport = viewport.D3DViewport();
        mList->RSSetViewports(1, &d3dViewport);
    }

    void DirectCommandListBase::TransitionResourceState(const ResourceTransitionBarrier& barrier)
    {
        mList->ResourceBarrier(1, &barrier.D3DBarrier());
    }

    void DirectCommandListBase::SetRenderTarget(const RTDescriptor& rtDescriptor, const DSDescriptor* depthStencilDescriptor)
    {
        const D3D12_CPU_DESCRIPTOR_HANDLE* dsHandle = depthStencilDescriptor ? &depthStencilDescriptor->CPUHandle() : nullptr;
        mList->OMSetRenderTargets(1, &rtDescriptor.CPUHandle(), false, dsHandle);
    }

    void DirectCommandListBase::ClearRenderTarget(const RTDescriptor& rtDescriptor, const Foundation::Color& color)
    {
        mList->ClearRenderTargetView(rtDescriptor.CPUHandle(), color.Ptr(), 0, nullptr);
    }

    void DirectCommandListBase::SetFence(const Fence& fence)
    {
        //mList->
    }

    CopyCommandList::CopyCommandList(Device& device, CopyCommandAllocator& allocator)
        : CopyCommandListBase(device, allocator, D3D12_COMMAND_LIST_TYPE_COPY) {}



    ComputeCommandList::ComputeCommandList(Device& device, ComputeCommandAllocator& allocator)
        : ComputeCommandListBase(device, allocator, D3D12_COMMAND_LIST_TYPE_COMPUTE) {}

    void ComputeCommandList::SetPipelineState(const ComputePipelineState& state)
    {

    }



    BundleCommandList::BundleCommandList(Device& device, BundleCommandAllocator& allocator)
        : DirectCommandListBase(device, allocator, D3D12_COMMAND_LIST_TYPE_BUNDLE) {}



    DirectCommandList::DirectCommandList(const Device& device, const DirectCommandAllocator& allocator)
        : DirectCommandListBase(device, allocator, D3D12_COMMAND_LIST_TYPE_DIRECT) {}

    void DirectCommandList::ExecuteBundle(const BundleCommandList& bundle)
    {

    }

    void DirectCommandList::SetPipelineState(const GraphicsPipelineState& state)
    {
        mList->SetPipelineState(state.D3DState());
        mList->SetGraphicsRootSignature(state.AssosiatedRootSignature().D3DSignature());
    }

    void DirectCommandList::Draw(uint32_t vertexCount, uint32_t vertexStart)
    {
        mList->DrawInstanced(vertexCount, 1, vertexStart, 0);
    }

    void DirectCommandList::DrawInstanced(uint32_t vertexCount, uint32_t vertexStart, uint32_t instanceCount)
    {
        mList->DrawInstanced(vertexCount, instanceCount, vertexStart, 1);
    }

    void DirectCommandList::DrawIndexed(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart)
    {
        mList->DrawIndexedInstanced(indexCount, 1, indexStart, vertexStart, 0);
    }

    void DirectCommandList::DrawIndexedInstanced(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart, uint32_t instanceCount)
    {
        mList->DrawIndexedInstanced(indexCount, instanceCount, indexStart, vertexStart, 1);
    }

}