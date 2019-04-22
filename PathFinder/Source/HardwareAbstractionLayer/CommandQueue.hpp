#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "CommandListTypeResolver.hpp"
#include "Device.hpp"

namespace HAL
{
    template <class CommmandListT>
    class CommandQueue
    {
    public:
        CommandQueue(const Device& device);

    private:
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> mQueue;

    public:
        inline const auto D3DPtr() const { return mQueue.Get(); }
    };

    template <class CommmandListT>
    CommandQueue<CommmandListT>::CommandQueue(const Device& device)
    {
        D3D12_COMMAND_QUEUE_DESC desc;
        desc.Type = CommandListTypeResolver<CommmandListT>::ListType();
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        ThrowIfFailed(device.D3DPtr()->CreateCommandQueue(desc, IID_PPV_ARGS(&mQueue)));
    }

}