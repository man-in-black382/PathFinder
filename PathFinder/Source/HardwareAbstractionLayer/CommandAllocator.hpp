#pragma once

#include <d3d12.h>
#include <wrl.h>

#include "CommandListTypeResolver.hpp"
#include "Device.hpp"

namespace HAL
{
    template <class CommandListT>
	class CommandAllocator
	{
    public:
        CommandAllocator(const Device& device);

    private:
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mAllocator;

    public:
        inline const auto Allocator() const { return mAllocator.Get(); }
	};

    template <class CommandListT>
    CommandAllocator<CommandListT>::CommandAllocator(const Device& device)
    {
        ThrowIfFailed(device.Device()->CreateCommandAllocator(CommandListTypeResolver<CommandListT>::ListType()), IID_PPV_ARGS(&mAllocator));
    }

}

