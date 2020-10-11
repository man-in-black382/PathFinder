#include "CommandAllocator.hpp"
#include "Utils.h"

#include <Foundation/StringUtils.hpp>

namespace HAL
{

    CommandAllocator::CommandAllocator(const Device& device, D3D12_COMMAND_LIST_TYPE commandListType)
    {
        ThrowIfFailed(device.D3DDevice()->CreateCommandAllocator(commandListType, IID_PPV_ARGS(&mAllocator)));
    }

    CommandAllocator::~CommandAllocator() {}

    void CommandAllocator::Reset()
    {
        ThrowIfFailed(mAllocator->Reset());
    }

    void CommandAllocator::SetDebugName(const std::string& name)
    {
        mAllocator->SetName(s2ws(name).c_str());
    }

    GraphicsCommandAllocator::GraphicsCommandAllocator(const Device& device)
        : CommandAllocator(device, D3D12_COMMAND_LIST_TYPE_DIRECT) {}

    CopyCommandAllocator::CopyCommandAllocator(const Device& device)
        : CommandAllocator(device, D3D12_COMMAND_LIST_TYPE_COPY) {}

    ComputeCommandAllocator::ComputeCommandAllocator(const Device& device)
        : CommandAllocator(device, D3D12_COMMAND_LIST_TYPE_COMPUTE) {}

    BundleCommandAllocator::BundleCommandAllocator(const Device& device)
        : CommandAllocator(device, D3D12_COMMAND_LIST_TYPE_BUNDLE) {}

}