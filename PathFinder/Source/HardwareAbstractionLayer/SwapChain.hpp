#pragma once

#include <dxgi.h>
#include <vector>

#include "CommandQueue.hpp"
#include "Resource.hpp"

namespace HAL
{
    enum class BackBufferingStrategy { Double = 2, Triple = 3 };

    class SwapChain
    {
    public:
        SwapChain(const CommandQueue<DirectCommandList>& commandQueue, HWND windowHandle, BackBufferingStrategy strategy);

        Resource& ResourceForBackBuffer(uint8_t buffer);

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory> mDXGIFactory;
        Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;

        std::vector<Resource> mBackBufferResources;

    public:

    };
}

