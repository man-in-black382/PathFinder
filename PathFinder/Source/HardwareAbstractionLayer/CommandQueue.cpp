#include "CommandQueue.hpp"
#include "Utils.h"

namespace HAL
{

    CommandQueue::CommandQueue(const Device& device, D3D12_COMMAND_LIST_TYPE commandListType)
    {
        D3D12_COMMAND_QUEUE_DESC desc{};
        desc.Type = commandListType;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        ThrowIfFailed(device.D3DPtr()->CreateCommandQueue(&desc, IID_PPV_ARGS(&mQueue)));
    }

    CommandQueue::~CommandQueue() {}

    void CommandQueue::StallCPUUntilDone(Fence& fence)
    {
        fence.IncreaseExpectedValue();
        mQueue->Signal(fence.D3DPtr(), fence.ExpectedValue());

        if (!fence.IsCompleted())
        {
            HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
            // Fire event when GPU hits current fence.  
            fence.SetCompletionEventHandle(eventHandle);
            // Wait until the GPU hits current fence event is fired.
            WaitForSingleObject(eventHandle, INFINITE);
            CloseHandle(eventHandle);
        }
    }



    DirectCommandQueue::DirectCommandQueue(const Device& device)
        : CommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT) {}

    void DirectCommandQueue::ExecuteCommandList(const DirectCommandList& list)
    {
        auto ptr = list.D3DList();
        mQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&ptr);
    }



    CopyCommandQueue::CopyCommandQueue(const Device& device)
        : CommandQueue(device, D3D12_COMMAND_LIST_TYPE_COPY) {}

    void CopyCommandQueue::ExecuteCommandList(const CopyCommandList& list)
    {
        auto ptr = list.D3DList();
        mQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&ptr);
    }

}