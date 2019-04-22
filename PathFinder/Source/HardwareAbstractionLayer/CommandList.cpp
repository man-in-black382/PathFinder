#include "CommandList.hpp"
#include "Utils.h"

namespace HAL
{

    CommandList::CommandList(ID3D12Device* device, ID3D12CommandAllocator* allocator, D3D12_COMMAND_LIST_TYPE type)
    {
        ThrowIfFailed(device->CreateCommandList(0, type, allocator, nullptr, IID_PPV_ARGS(&mList)));
        mList->Close();
    }

    DirectCommandList::DirectCommandList(Device& device, CommandAllocator<DirectCommandList>& allocator)
        : CommandList(device.D3DPtr(), allocator.D3DPtr(), CommandListTypeResolver<DirectCommandList>::ListType())
    {

    }

    BundleCommandList::BundleCommandList(Device& device, CommandAllocator<BundleCommandList>& allocator)
        : CommandList(device.D3DPtr(), allocator.D3DPtr(), CommandListTypeResolver<BundleCommandList>::ListType())
    {

    }

    CopyCommandList::CopyCommandList(Device& device, CommandAllocator<CopyCommandList>& allocator)
        : CommandList(device.D3DPtr(), allocator.D3DPtr(), CommandListTypeResolver<CopyCommandList>::ListType())
    {

    }

    ComputeCommandList::ComputeCommandList(Device& device, CommandAllocator<ComputeCommandList>& allocator)
        : CommandList(device.D3DPtr(), allocator.D3DPtr(), CommandListTypeResolver<ComputeCommandList>::ListType())
    {

    }

    VideoProcessingCommandList::VideoProcessingCommandList(Device& device, CommandAllocator<VideoProcessingCommandList>& allocator)
        : CommandList(device.D3DPtr(), allocator.D3DPtr(), CommandListTypeResolver<VideoProcessingCommandList>::ListType())
    {

    }

    VideoDecodingCommandList::VideoDecodingCommandList(Device& device, CommandAllocator<VideoDecodingCommandList>& allocator)
        : CommandList(device.D3DPtr(), allocator.D3DPtr(), CommandListTypeResolver<VideoDecodingCommandList>::ListType())
    {

    }

}