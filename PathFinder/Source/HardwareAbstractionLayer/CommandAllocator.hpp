#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "Device.hpp"

namespace HAL
{
    class CommandAllocator
    {
    public:
        CommandAllocator(const Device& device, D3D12_COMMAND_LIST_TYPE commandListType);
        virtual ~CommandAllocator() = 0;

    private:
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mAllocator;

    public:
        inline const auto D3DPtr() const { return mAllocator.Get(); }
    };


    class DirectCommandAllocator : public CommandAllocator {
    public:
        DirectCommandAllocator(const Device& device);
        ~DirectCommandAllocator() = default;
    };

    //class BundleCommandAllocator : public CommandAllocator {
    //public:
    //    BundleCommandAllocator(const Device& device);
    //};

    //class CopyCommandAllocator : public CommandAllocator {
    //public:
    //    CopyCommandAllocator(const Device& device);
    //};

    //class ComputeCommandAllocator : public CommandAllocator {
    //public:
    //    ComputeCommandAllocator(const Device& device);
    //};

    //class VideoProcessingCommandAllocator : public CommandAllocator {
    //public:
    //    VideoProcessingCommandAllocator(const Device& device);
    //};

    //class VideoDecodingCommandAllocator : public CommandAllocator {
    //public:
    //    VideoDecodingCommandAllocator(const Device& device);
    //};

}

