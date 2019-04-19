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
        CommandList(const ID3D12Device* device, const ID3D12CommandAllocator* allocator, D3D12_COMMAND_LIST_TYPE type);

    private:
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mList;
    };

    class DirectCommandList : public CommandList {
    public:
        DirectCommandList(const Device& device, const CommandAllocator<DirectCommandList>& allocator);
    };

    class BundleCommandList : public CommandList {
    public:
        BundleCommandList(const Device& device, const CommandAllocator<BundleCommandList>& allocator);
    };

    class CopyCommandList : public CommandList {
    public:
        CopyCommandList(const Device& device, const CommandAllocator<CopyCommandList>& allocator);
    };

    class ComputeCommandList : public CommandList {
    public:
        ComputeCommandList(const Device& device, const CommandAllocator<ComputeCommandList>& allocator);
    };

    class VideoProcessingCommandList : public CommandList {
    public:
        VideoProcessingCommandList(const Device& device, const CommandAllocator<VideoProcessingCommandList>& allocator);
    };

    class VideoDecodingCommandList : public CommandList {
    public:
        VideoDecodingCommandList(const Device& device, const CommandAllocator<VideoDecodingCommandList>& allocator);
    };

}

