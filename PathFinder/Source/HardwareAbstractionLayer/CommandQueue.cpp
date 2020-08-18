#include "CommandQueue.hpp"
#include "Utils.h"

#include "../Foundation/StringUtils.hpp"

namespace HAL
{

    CommandQueue::CommandQueue(const Device& device, D3D12_COMMAND_LIST_TYPE commandListType)
    {
        D3D12_COMMAND_QUEUE_DESC desc{};
        desc.Type = commandListType;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        ThrowIfFailed(device.D3DDevice()->CreateCommandQueue(&desc, IID_PPV_ARGS(&mQueue)));
    }

    CommandQueue::~CommandQueue() {}

    void CommandQueue::SignalFence(const Fence& fence, std::optional<uint64_t> explicitFenceValue)
    {
        mQueue->Signal(fence.D3DFence(), explicitFenceValue.value_or(fence.ExpectedValue()));
    }

    void CommandQueue::WaitFence(const Fence& fence, std::optional<uint64_t> explicitFenceValue)
    {
        mQueue->Wait(fence.D3DFence(), explicitFenceValue.value_or(fence.ExpectedValue()));
    }

    void CommandQueue::SetDebugName(const std::string& name)
    {
        mQueue->SetName(StringToWString(name).c_str());
    }



    GraphicsCommandQueue::GraphicsCommandQueue(const Device& device)
        : CommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT) {}

    void GraphicsCommandQueue::ExecuteCommandList(const GraphicsCommandList& list)
    {
        auto ptr = list.D3DList();
        mQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&ptr);
    }

    void GraphicsCommandQueue::ExecuteCommandLists(const GraphicsCommandList* const* lists, uint64_t count)
    {
        ExecuteCommandListsInternal(lists, count);
    }



    ComputeCommandQueue::ComputeCommandQueue(const Device& device)
        : CommandQueue(device, D3D12_COMMAND_LIST_TYPE_COMPUTE) {}

    void ComputeCommandQueue::ExecuteCommandList(const ComputeCommandList& list)
    {
        auto ptr = list.D3DList();
        mQueue->ExecuteCommandLists(1, (ID3D12CommandList * const*)& ptr);
    }

    void ComputeCommandQueue::ExecuteCommandLists(const ComputeCommandList* const* lists, uint64_t count)
    {
        ExecuteCommandListsInternal(lists, count);
    }



    CopyCommandQueue::CopyCommandQueue(const Device& device)
        : CommandQueue(device, D3D12_COMMAND_LIST_TYPE_COPY) {}

    void CopyCommandQueue::ExecuteCommandList(const CopyCommandList& list)
    {
        auto ptr = list.D3DList();
        mQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&ptr);
    }

    void CopyCommandQueue::ExecuteCommandLists(const CopyCommandList* const* lists, uint64_t count)
    {
        ExecuteCommandListsInternal(lists, count);
    }

}