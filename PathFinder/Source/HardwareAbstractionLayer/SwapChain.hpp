#pragma once

#include <dxgi1_6.h>
#include <vector>
#include <cstdint>
#include <memory>

#include "CommandQueue.hpp"
#include "Resource.hpp"
#include "Display.hpp"
#include "ResourceFormat.hpp"

#include <Geometry/Dimensions.hpp>

namespace HAL
{
    enum class BackBufferingStrategy: uint8_t
    { 
        Double = 2, Triple = 3 
    };

    class SwapChain
    {
    public:
        inline static const ColorFormat SDRBackBufferFormat = ColorFormat::RGBA8_Usigned_Norm;
        inline static const ColorFormat HDRBackBufferFormat = ColorFormat::RGB10A2_Unorm;

        SwapChain(
            const Display* display,
            const GraphicsCommandQueue& commandQueue,
            HWND windowHandle, 
            bool enableHDRIfAvailable,
            BackBufferingStrategy strategy,
            const Geometry::Dimensions& dimensions
        );

        void SetDisplay(const Display* display, bool enableHDRIfAvailable);
        void SetDimensions(const Geometry::Dimensions& dimensions);
        void Present();

    private:
        void CreateD3DSwapChain(const DXGI_SWAP_CHAIN_DESC1& desc);
        ColorFormat BackBufferFormatForSpace(ColorSpace space, bool preferHDR) const;

        Microsoft::WRL::ComPtr<IDXGIFactory4> mDXGIFactory;
        Microsoft::WRL::ComPtr<IDXGISwapChain4> mSwapChain;
        HWND mWindowHandle;
        ID3D12CommandQueue* mQueue = nullptr;
        bool mAreBackBuffersUpdated = false;
        ColorSpace mCurrentColorSpace = ColorSpace::Rec709;

        std::vector<std::unique_ptr<Texture>> mBackBuffers;

    public:
        inline bool AreBackBuffersUpdated() const { return mAreBackBuffersUpdated; }
        inline auto& BackBuffers() { return mBackBuffers; }
        inline const auto& BackBuffers() const { return mBackBuffers; }
    };
}

