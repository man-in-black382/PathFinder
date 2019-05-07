#include "CommandAllocator.hpp"
#include "Utils.h"

namespace HAL
{

    CommandAllocator::CommandAllocator(const Device& device, D3D12_COMMAND_LIST_TYPE commandListType)
    {
        ThrowIfFailed(device.D3DPtr()->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&mAllocator)));
    }

    CommandAllocator::~CommandAllocator() {}

    DirectCommandAllocator::DirectCommandAllocator(const Device& device)
        : CommandAllocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT) {}

}