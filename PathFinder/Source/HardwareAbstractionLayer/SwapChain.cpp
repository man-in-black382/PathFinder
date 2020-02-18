#include "SwapChain.hpp"
#include "Utils.h"

namespace HAL
{

    SwapChain::SwapChain(
        const GraphicsCommandQueue& commandQueue,
        HWND windowHandle,
        BackBufferingStrategy strategy,
        ColorFormat backBufferFormat,
        const Geometry::Dimensions& dimensions)
    {
        DXGI_SWAP_CHAIN_DESC chain{};

        uint8_t bufferCount = std::underlying_type<BackBufferingStrategy>::type(strategy);

        chain.BufferDesc.Width = (UINT)dimensions.Width;
        chain.BufferDesc.Height = (UINT)dimensions.Height;
        chain.BufferDesc.Format = ResourceFormat::D3DFormat(backBufferFormat);
        chain.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        chain.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        chain.SampleDesc.Count = 1;
        chain.SampleDesc.Quality = 0;
        chain.BufferUsage = DXGI_USAGE_BACK_BUFFER;
        chain.BufferCount = bufferCount;
        chain.OutputWindow = windowHandle;
        chain.Windowed = true;
        chain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        chain.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        ThrowIfFailed(CreateDXGIFactory(IID_PPV_ARGS(&mDXGIFactory)));
        ThrowIfFailed(mDXGIFactory->CreateSwapChain(commandQueue.D3DPtr(), &chain, &mSwapChain));

        Microsoft::WRL::ComPtr<ID3D12Resource> backBufferResourcePtr;

        for (int bufferIdx = 0; bufferIdx < bufferCount; bufferIdx++)
        {
            ThrowIfFailed(mSwapChain->GetBuffer(bufferIdx, IID_PPV_ARGS(&backBufferResourcePtr)));
            mBackBuffers.emplace_back(std::make_unique<Texture>(backBufferResourcePtr));
            mBackBuffers.back()->SetDebugName("Back Buffer " + std::to_string(bufferIdx));
        }
    }

    void SwapChain::Present()
    {
        ThrowIfFailed(mSwapChain->Present(1, 0));
    }

}
