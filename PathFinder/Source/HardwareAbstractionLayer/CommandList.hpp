#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "CommandListTypeResolver.hpp"
#include "Device.hpp"
#include "CommandAllocator.hpp"

namespace HAL
{
    class CommandList
    {
    protected:
        CommandList(ID3D12Device* device, ID3D12CommandAllocator* allocator, D3D12_COMMAND_LIST_TYPE type);

    private:
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mList;
    };

    class DirectCommandList : public CommandList {
    public:
        DirectCommandList(Device& device, CommandAllocator<DirectCommandList>& allocator);
    };

    class BundleCommandList : public CommandList {
    public:
        BundleCommandList(Device& device, CommandAllocator<BundleCommandList>& allocator);
    };

    class CopyCommandList : public CommandList {
    public:
        CopyCommandList(Device& device, CommandAllocator<CopyCommandList>& allocator);
    };

    class ComputeCommandList : public CommandList {
    public:
        ComputeCommandList(Device& device, CommandAllocator<ComputeCommandList>& allocator);
    };

    class VideoProcessingCommandList : public CommandList {
    public:
        VideoProcessingCommandList(Device& device, CommandAllocator<VideoProcessingCommandList>& allocator);
    };

    class VideoDecodingCommandList : public CommandList {
    public:
        VideoDecodingCommandList(Device& device, CommandAllocator<VideoDecodingCommandList>& allocator);
    };

}

