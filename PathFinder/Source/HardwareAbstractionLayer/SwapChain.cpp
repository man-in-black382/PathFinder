#include "SwapChain.hpp"
#include "Utils.h"

namespace HAL
{

    SwapChain::SwapChain(
        const Display* display,
        const GraphicsCommandQueue& commandQueue,
        HWND windowHandle,
        bool enableHDRIfAvailable,
        BackBufferingStrategy strategy,
        const Geometry::Dimensions& dimensions)
        :
        mWindowHandle{ windowHandle },
        mQueue{ commandQueue.D3DQueue() }
    {
        ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&mDXGIFactory)));

        DXGI_SWAP_CHAIN_DESC1 desc{};

        uint8_t bufferCount = std::underlying_type<BackBufferingStrategy>::type(strategy);

        desc.Width = (UINT)dimensions.Width;
        desc.Height = (UINT)dimensions.Height;
        desc.Format = D3DFormat(BackBufferFormatForSpace(display->DisplayColorSpace(), enableHDRIfAvailable));
        desc.Scaling = DXGI_SCALING_NONE;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
        desc.BufferCount = bufferCount;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        CreateD3DSwapChain(desc);

        mCurrentColorSpace = display->DisplayColorSpace();
        mSwapChain->SetColorSpace1(D3DColorSpace(mCurrentColorSpace));
    }

    void SwapChain::SetDisplay(const Display* display, bool enableHDRIfAvailable)
    {
        DXGI_COLOR_SPACE_TYPE newSpace = D3DColorSpace(display->DisplayColorSpace());
        DXGI_FORMAT newFormat = D3DFormat(BackBufferFormatForSpace(display->DisplayColorSpace(), enableHDRIfAvailable));

        DXGI_SWAP_CHAIN_DESC1 desc{};
        mSwapChain->GetDesc1(&desc);

        if (display->DisplayColorSpace() == mCurrentColorSpace && newFormat == desc.Format)
            return;

        mCurrentColorSpace = display->DisplayColorSpace();

        desc.Format = newFormat;
        CreateD3DSwapChain(desc);
        mSwapChain->SetColorSpace1(newSpace);
    }

    void SwapChain::SetDimensions(const Geometry::Dimensions& dimensions)
    {
        DXGI_SWAP_CHAIN_DESC1 desc{};
        mSwapChain->GetDesc1(&desc);
        
        if (desc.Width == dimensions.Width && desc.Height == dimensions.Height)
            return;

        desc.Width = dimensions.Width;
        desc.Height = dimensions.Height;

        CreateD3DSwapChain(desc);
    }

    void SwapChain::CreateD3DSwapChain(const DXGI_SWAP_CHAIN_DESC1& desc)
    {
        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(mDXGIFactory->CreateSwapChainForHwnd(mQueue, mWindowHandle, &desc, nullptr, nullptr, &swapChain));
        ThrowIfFailed(swapChain.As(&mSwapChain));

        Microsoft::WRL::ComPtr<ID3D12Resource> backBufferResourcePtr;

        mBackBuffers.clear();

        for (auto bufferIdx = 0u; bufferIdx < desc.BufferCount; bufferIdx++)
        {
            ThrowIfFailed(mSwapChain->GetBuffer(bufferIdx, IID_PPV_ARGS(&backBufferResourcePtr)));
            mBackBuffers.emplace_back(std::make_unique<Texture>(backBufferResourcePtr));
            mBackBuffers.back()->SetDebugName("Back Buffer " + std::to_string(bufferIdx));
        }

        mAreBackBuffersUpdated = true;
    }

    ColorFormat SwapChain::BackBufferFormatForSpace(ColorSpace space, bool preferHDR) const
    {
        switch (space)
        {
        case HAL::ColorSpace::Rec709: return SDRBackBufferFormat;
        case HAL::ColorSpace::Rec2020: return preferHDR ? HDRBackBufferFormat : SDRBackBufferFormat;
        default: return SDRBackBufferFormat;
        }
    }

    void SwapChain::Present()
    {
        ThrowIfFailed(mSwapChain->Present(1, 0));
        mAreBackBuffersUpdated = false;
    }

}
