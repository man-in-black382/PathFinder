#include "CommandQueue.hpp"
#include "Utils.h"

namespace HAL
{

    CommandQueue::CommandQueue(const Device& device, D3D12_COMMAND_LIST_TYPE commandListType)
    {
        D3D12_COMMAND_QUEUE_DESC desc;
        desc.Type = commandListType;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        ThrowIfFailed(device.D3DPtr()->CreateCommandQueue(&desc, IID_PPV_ARGS(&mQueue)));
    }

    CommandQueue::~CommandQueue() {}

    DirectCommandQueue::DirectCommandQueue(const Device& device)
        : CommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT) {}

}