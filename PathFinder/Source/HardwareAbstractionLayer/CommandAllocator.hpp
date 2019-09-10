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

