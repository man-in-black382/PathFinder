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

    //void GraphicsDevice::SetRenderTarget(const ResourceView<HAL::RTDescriptor>& view)
    //{
    //    /*mCommandList.SetRenderTarget(view.ResourceDescriptor());*/
    //}

    void GraphicsDevice::SetRenderTarget(Foundation::Name resourceName)
    {
        mCommandList.SetRenderTarget(mResourceManager->GetRenderTarget(resourceName).ResourceDescriptor());
    }

    void GraphicsDevice::SetBackBufferAsRenderTarget()
    {
        mCommandList.SetRenderTarget(mResourceManager->GetBackBuffer().ResourceDescriptor());
    }

    /*void GraphicsDevice::SetRenderTargetAndDepthStencil(const ResourceView<HAL::RTDescriptor>& rtView, const ResourceView<HAL::DSDescriptor>& dsView)
    {
        mCommandList.SetRenderTarget(rtView.ResourceDescriptor(), dsView.ResourceDescriptor());
    }*/

    void GraphicsDevice::SetRenderTargetAndDepthStencil(Foundation::Name rtResourceName, Foundation::Name dsResourceName)
    {
        mCommandList.SetRenderTarget(
            mResourceManager->GetRenderTarget(rtResourceName).ResourceDescriptor(),
            mResourceManager->GetDepthStencil(dsResourceName).ResourceDescriptor()
        );
    }

    //void GraphicsDevice::ClearRenderTarget(const Foundation::Color& color, const ResourceView<HAL::RTDescriptor>& rtView)
    //{
    //    mCommandList.ClearRenderTarget(rtView.ResourceDescriptor(), color);
    //}

    void GraphicsDevice::ClearRenderTarget(Foundation::Name resourceName, const Foundation::Color& color)
    {
        mCommandList.ClearRenderTarget(mResourceManager->GetRenderTarget(resourceName).ResourceDescriptor(), color);
    }

    void GraphicsDevice::ClearBackBuffer(const Foundation::Color& color)
    {
        mCommandList.ClearRenderTarget(mResourceManager->GetBackBuffer().ResourceDescriptor(), color);
    }

    /* void GraphicsDevice::ClearDepthStencil(float depthValue, const ResourceView<HAL::DSDescriptor>& dsView)
     {
         mCommandList.CleadDepthStencil(dsView.ResourceDescriptor(), depthValue);
     }*/

    void GraphicsDevice::ClearDepthStencil(Foundation::Name resourceName, float depthValue)
    {
        mCommandList.CleadDepthStencil(mResourceManager->GetDepthStencil(resourceName).ResourceDescriptor(), depthValue);
    }

    /* void GraphicsDevice::ApplyPipelineState(const HAL::GraphicsPipelineState& state)
     {
         mCommandList.SetPipelineState(state);
     }*/

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
