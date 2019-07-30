#include "GraphicsDevice.hpp"

namespace PathFinder
{

    GraphicsDevice::GraphicsDevice(
        HAL::Device* device, HWND windowHandle, const RenderSurface& renderSurface,
        ResourceManager* resourceManager, PipelineStateManager* pipelineStateManager
    )
        : mDevice{ device },
        mCommandAllocator{ *device },
        mCommandList{ *device, mCommandAllocator },
        mCommandQueue{ *device },
        mFence{ *device },
        mSwapChain{ mCommandQueue, windowHandle, HAL::BackBufferingStrategy::Double,
                renderSurface.RenderTargetFormat(), renderSurface.Dimensions() },
        mResourceManager{ resourceManager },
        mPipelineStateManager{ pipelineStateManager } {}


    void GraphicsDevice::SetRenderTarget(Foundation::Name resourceName)
    {
        mCommandList.SetRenderTarget(mResourceManager->GetRenderTarget(resourceName));
    }

    void GraphicsDevice::SetBackBufferAsRenderTarget(std::optional<Foundation::Name> depthStencilResourceName)
    {
        if (depthStencilResourceName)
        {
            mCommandList.SetRenderTarget(
                mResourceManager->GetBackBuffer(),
                mResourceManager->GetDepthStencil(*depthStencilResourceName)
            );
        }
        else {
            mCommandList.SetRenderTarget(mResourceManager->GetBackBuffer());
        }
    }

    void GraphicsDevice::SetRenderTargetAndDepthStencil(Foundation::Name rtResourceName, Foundation::Name dsResourceName)
    {
        mCommandList.SetRenderTarget(
            mResourceManager->GetRenderTarget(rtResourceName),
            mResourceManager->GetDepthStencil(dsResourceName)
        );
    }

    void GraphicsDevice::ClearRenderTarget(Foundation::Name resourceName, const Foundation::Color& color)
    {
        mCommandList.ClearRenderTarget(mResourceManager->GetRenderTarget(resourceName), color);
    }

    void GraphicsDevice::ClearBackBuffer(const Foundation::Color& color)
    {
        mCommandList.ClearRenderTarget(mResourceManager->GetBackBuffer(), color);
    }

    void GraphicsDevice::ClearDepthStencil(Foundation::Name resourceName, float depthValue)
    {
        mCommandList.CleadDepthStencil(mResourceManager->GetDepthStencil(resourceName), depthValue);
    }

    void GraphicsDevice::ApplyPipelineState(Foundation::Name psoName)
    {
        mCommandList.SetPipelineState(mPipelineStateManager->GetPipelineState(psoName));
    }

    void GraphicsDevice::Draw(uint32_t vertexCount, uint32_t vertexStart)
    {
        mCommandList.Draw(vertexCount, vertexStart);
    }

    void GraphicsDevice::DrawInstanced(uint32_t vertexCount, uint32_t vertexStart, uint32_t instanceCount)
    {
        mCommandList.DrawInstanced(vertexCount, vertexStart, instanceCount);
    }

    void GraphicsDevice::DrawIndexed(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart)
    {
        mCommandList.DrawIndexed(vertexStart, indexCount, indexStart);
    }

    void GraphicsDevice::DrawIndexedInstanced(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart, uint32_t instanceCount)
    {
        mCommandList.DrawIndexedInstanced(vertexStart, indexCount, indexStart, instanceCount);
    }

    void GraphicsDevice::TransitionResource(const HAL::ResourceTransitionBarrier& barrier)
    {
        mCommandList.TransitionResourceState(barrier);
    }

    void GraphicsDevice::ExecuteCommandBuffer()
    {
        mCommandList.Close();
        mCommandQueue.ExecuteCommandList(mCommandList);
        mCommandQueue.StallCPUUntilDone(mFence);
        mCommandAllocator.Reset();
        mCommandList.Reset(mCommandAllocator);
    }

}
