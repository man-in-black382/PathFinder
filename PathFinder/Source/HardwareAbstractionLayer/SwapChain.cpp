#include "SwapChain.hpp"
#include "Utils.h"

namespace HAL
{

    SwapChain::SwapChain(const CommandQueue<DirectCommandList>& commandQueue, HWND windowHandle, BackBufferingStrategy strategy, const Geometry::Dimensions& dimensions)
    {
        DXGI_SWAP_CHAIN_DESC chain;

        chain.BufferDesc.Width = dimensions.Width;
        chain.BufferDesc.Height = dimensions.Height;
        chain.BufferDesc.RefreshRate.Numerator = 60;
        chain.BufferDesc.RefreshRate.Denominator = 1;
        chain.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        chain.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        chain.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        chain.SampleDesc.Count = 1;
        chain.SampleDesc.Quality = 0;
        chain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        chain.BufferCount = std::underlying_type<BackBufferingStrategy>::type(strategy);
        chain.OutputWindow = windowHandle;
        chain.Windowed = true;
        chain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        chain.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&mDXGIFactory)));
        ThrowIfFailed(mDXGIFactory->CreateSwapChain(commandQueue.D3DPtr(), &chain, mSwapChain.GetAddressOf()));

        Microsoft::WRL::ComPtr<ID3D12Resource> backBufferResourcePtr;
        ThrowIfFailed(mSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferResourcePtr)));
        mBackBuffer = std::make_unique<ColorTextureResource>(backBufferResourcePtr);
    }

}
