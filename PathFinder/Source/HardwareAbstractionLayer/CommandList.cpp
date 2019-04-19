#include "CommandList.hpp"
#include "Utils.h"

namespace HAL
{

    CommandList::CommandList(const ID3D12Device* device, const ID3D12CommandAllocator* allocator, D3D12_COMMAND_LIST_TYPE type)
    {
        ThrowIfFailed(device->CreateCommandList(0, type, allocator, nullptr, IID_PPV_ARGS(&mList)));
        mList->Close();
    }

    DirectCommandList::DirectCommandList(const Device& device, const CommandAllocator<DirectCommandList>& allocator)
        : CommandList(device.Device(), allocator.Allocator(), CommandListTypeResolver<DirectCommandList>::ListType())
    {

    }

    BundleCommandList::BundleCommandList(const Device& device, const CommandAllocator<BundleCommandList>& allocator)
        : CommandList(device.Device(), allocator.Allocator(), CommandListTypeResolver<BundleCommandList>::ListType())
    {

    }

    CopyCommandList::CopyCommandList(const Device& device, const CommandAllocator<CopyCommandList>& allocator)
        : CommandList(device.Device(), allocator.Allocator(), CommandListTypeResolver<CopyCommandList>::ListType())
    {

    }

    ComputeCommandList::ComputeCommandList(const Device& device, const CommandAllocator<ComputeCommandList>& allocator)
        : CommandList(device.Device(), allocator.Allocator(), CommandListTypeResolver<ComputeCommandList>::ListType())
    {

    }

    VideoProcessingCommandList::VideoProcessingCommandList(const Device& device, const CommandAllocator<VideoProcessingCommandList>& allocator)
        : CommandList(device.Device(), allocator.Allocator(), CommandListTypeResolver<VideoProcessingCommandList>::ListType())
    {

    }

    VideoDecodingCommandList::VideoDecodingCommandList(const Device& device, const CommandAllocator<VideoDecodingCommandList>& allocator)
        : CommandList(device.Device(), allocator.Allocator(), CommandListTypeResolver<VideoDecodingCommandList>::ListType())
    {

    }

}