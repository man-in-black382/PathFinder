#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "Device.hpp"
#include "CommandList.hpp"

namespace HAL
{

    class CommandQueue
    {
    public:
        CommandQueue(const Device& device, D3D12_COMMAND_LIST_TYPE commandListType);
        virtual ~CommandQueue() = 0;

    protected:
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> mQueue;

    public:
        inline const auto D3DPtr() const { return mQueue.Get(); }
    };


    class DirectCommandQueue : public CommandQueue {
    public:
        DirectCommandQueue(const Device& device);
        ~DirectCommandQueue() = default;

        void ExecuteCommandList(const DirectCommandList& list);
    };

}