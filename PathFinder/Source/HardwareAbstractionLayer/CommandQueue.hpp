#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "GraphicAPIObject.hpp"
#include "Device.hpp"
#include "CommandList.hpp"
#include "Fence.hpp"

namespace HAL
{

    class CommandQueue : public GraphicAPIObject
    {
    public:
        CommandQueue(const Device& device, D3D12_COMMAND_LIST_TYPE commandListType);
        virtual ~CommandQueue() = 0;

        void SignalFence(const Fence& fence, std::optional<uint64_t> explicitFenceValue = std::nullopt);
        void WaitFence(const Fence& fence, std::optional<uint64_t> explicitFenceValue = std::nullopt);
        void SetDebugName(const std::string& name) override;
        uint64_t GetTimestampFrequency() const;

    protected:
        template <class CommandListT>
        void ExecuteCommandListsInternal(const CommandListT* const* lists, uint64_t count);

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> mQueue;

    public:
        inline const auto D3DQueue() const { return mQueue.Get(); }
    };

    class GraphicsCommandQueue : public CommandQueue {
    public:
        GraphicsCommandQueue(const Device& device);
        ~GraphicsCommandQueue() = default;

        void ExecuteCommandList(const GraphicsCommandList& list);
        void ExecuteCommandLists(const GraphicsCommandList* const* lists, uint64_t count);
    };

    class ComputeCommandQueue : public CommandQueue {
    public:
        ComputeCommandQueue(const Device& device);
        ~ComputeCommandQueue() = default;

        void ExecuteCommandList(const ComputeCommandList& list);
        void ExecuteCommandLists(const ComputeCommandList* const* lists, uint64_t count);
    };

    class CopyCommandQueue : public CommandQueue {
    public:
        CopyCommandQueue(const Device& device);
        ~CopyCommandQueue() = default;

        void ExecuteCommandList(const CopyCommandList& list);
        void ExecuteCommandLists(const CopyCommandList* const* lists, uint64_t count);
    };

}

#include "CommandQueue.inl"