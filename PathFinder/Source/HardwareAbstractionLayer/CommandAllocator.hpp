#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "GraphicAPIObject.hpp"
#include "Device.hpp"

namespace HAL
{
    class CommandAllocator : public GraphicAPIObject
    {
    public:
        CommandAllocator(const Device& device, D3D12_COMMAND_LIST_TYPE commandListType);
        virtual ~CommandAllocator() = 0;

        void Reset();
        void SetDebugName(const std::string& name) override;

    private:
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mAllocator;

    public:
        inline const auto D3DPtr() const { return mAllocator.Get(); }
    };


    class GraphicsCommandAllocator : public CommandAllocator {
    public:
        GraphicsCommandAllocator(const Device& device);
        ~GraphicsCommandAllocator() = default;
    };

    class BundleCommandAllocator : public CommandAllocator {
    public:
        BundleCommandAllocator(const Device& device);
        ~BundleCommandAllocator() = default;
    };

    class CopyCommandAllocator : public CommandAllocator {
    public:
        CopyCommandAllocator(const Device& device);
        ~CopyCommandAllocator() = default;
    };

    class ComputeCommandAllocator : public CommandAllocator {
    public:
        ComputeCommandAllocator(const Device& device);
        ~ComputeCommandAllocator() = default;
    };

}

