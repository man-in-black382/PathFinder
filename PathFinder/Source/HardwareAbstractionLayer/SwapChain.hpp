#pragma once

#include <dxgi.h>
#include <vector>
#include <cstdint>
#include <memory>

#include "CommandQueue.hpp"
#include "Resource.hpp"

#include "../Geometry/Dimensions.hpp"

namespace HAL
{
    enum class BackBufferingStrategy: uint8_t { Double = 2, Triple = 3 };

    class SwapChain
    {
    public:
        SwapChain(
            const GraphicsCommandQueue& commandQueue,
            HWND windowHandle, 
            BackBufferingStrategy strategy,
            ResourceFormat::Color backBufferFormat,
            const Geometry::Dimensions& dimensions
        );

        void Present();

    private:
        Microsoft::WRL::ComPtr<IDXGIFactory> mDXGIFactory;
        Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;

        std::vector<std::unique_ptr<TextureResource>> mBackBuffers;

    public:
        inline auto& BackBuffers() { return mBackBuffers; }
        inline const auto& BackBuffers() const { return mBackBuffers; }
    };
}

