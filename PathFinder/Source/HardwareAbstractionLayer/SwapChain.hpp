#pragma once

#include <dxgi.h>
#include <vector>
#include <cstdint>

#include "CommandQueue.hpp"
#include "Resource.hpp"

namespace HAL
{
    enum class BackBufferingStrategy: uint8_t { Double = 2, Triple = 3 };

    class SwapChain
    {
    public:
        SwapChain(const CommandQueue<DirectCommandList>& commandQueue, HWND windowHandle, BackBufferingStrategy strategy);

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory> mDXGIFactory;
        Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;

        std::vector<Resource> mBackBufferResources;

    public:

    };
}

