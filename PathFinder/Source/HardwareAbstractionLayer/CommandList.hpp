#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "Device.hpp"
#include "CommandAllocator.hpp"
#include "Viewport.hpp"

namespace HAL
{
    class CommandList
    {
    public:
        CommandList(const Device& device, const DirectCommandAllocator& allocator, D3D12_COMMAND_LIST_TYPE type);
        virtual ~CommandList() = 0;

        void SetViewport(const Viewport& viewport);

    private:
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mList;
    };

    class DirectCommandList : public CommandList {
    public:
        DirectCommandList(const Device& device, const DirectCommandAllocator& allocator);
        ~DirectCommandList() = default;
    };

    //class BundleCommandList : public CommandList {
    //public:
    //    BundleCommandList(Device& device, CommandAllocator<BundleCommandList>& allocator);
    //};

    //class CopyCommandList : public CommandList {
    //public:
    //    CopyCommandList(Device& device, CommandAllocator<CopyCommandList>& allocator);
    //};

    //class ComputeCommandList : public CommandList {
    //public:
    //    ComputeCommandList(Device& device, CommandAllocator<ComputeCommandList>& allocator);
    //};

    //class VideoProcessingCommandList : public CommandList {
    //public:
    //    VideoProcessingCommandList(Device& device, CommandAllocator<VideoProcessingCommandList>& allocator);
    //};

    //class VideoDecodingCommandList : public CommandList {
    //public:
    //    VideoDecodingCommandList(Device& device, CommandAllocator<VideoDecodingCommandList>& allocator);
    //};

}

