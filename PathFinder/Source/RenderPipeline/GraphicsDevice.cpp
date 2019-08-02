#include "GraphicsDevice.hpp"

namespace PathFinder
{

    GraphicsDevice::GraphicsDevice(
        HAL::Device* device, HWND windowHandle, const RenderSurface& renderSurface,
        ResourceManager* resourceManager, PipelineStateManager* pipelineStateManager,
        VertexStorage* vertexStorage)
        : 
        mDevice{ device },
        mCommandQueue{ *device },
        mRingCommandList{ *device, 3 },
        mFence{ *device },
        mSwapChain{ mCommandQueue, windowHandle, HAL::BackBufferingStrategy::Double,
                renderSurface.RenderTargetFormat(), renderSurface.Dimensions() },
        mResourceManager{ resourceManager },
        mPipelineStateManager{ pipelineStateManager },
        mVertexStorage{ vertexStorage } {}


    void GraphicsDevice::SetRenderTarget(Foundation::Name resourceName)
    {
        mRingCommandList.CurrentCommandList().SetRenderTarget(mResourceManager->GetRenderTarget(resourceName));
    }

    void GraphicsDevice::SetBackBufferAsRenderTarget(std::optional<Foundation::Name> depthStencilResourceName)
    {
        if (depthStencilResourceName)
        {
            mRingCommandList.CurrentCommandList().SetRenderTarget(
                mResourceManager->GetBackBuffer(),
                mResourceManager->GetDepthStencil(*depthStencilResourceName)
            );
        }
        else {
            mRingCommandList.CurrentCommandList().SetRenderTarget(mResourceManager->GetBackBuffer());
        }
    }

    void GraphicsDevice::SetRenderTargetAndDepthStencil(Foundation::Name rtResourceName, Foundation::Name dsResourceName)
    {
        mRingCommandList.CurrentCommandList().SetRenderTarget(
            mResourceManager->GetRenderTarget(rtResourceName),
            mResourceManager->GetDepthStencil(dsResourceName)
        );
    }

    void GraphicsDevice::ClearRenderTarget(Foundation::Name resourceName, const Foundation::Color& color)
    {
        mRingCommandList.CurrentCommandList().ClearRenderTarget(mResourceManager->GetRenderTarget(resourceName), color);
    }

    void GraphicsDevice::ClearBackBuffer(const Foundation::Color& color)
    {
        mRingCommandList.CurrentCommandList().ClearRenderTarget(mResourceManager->GetBackBuffer(), color);
    }

    void GraphicsDevice::ClearDepth(Foundation::Name resourceName, float depthValue)
    {
        mRingCommandList.CurrentCommandList().CleadDepthStencil(mResourceManager->GetDepthStencil(resourceName), depthValue);
    }

    void GraphicsDevice::ApplyPipelineState(Foundation::Name psoName)
    {
        mRingCommandList.CurrentCommandList().SetPipelineState(mPipelineStateManager->GetPipelineState(psoName));
        mRingCommandList.CurrentCommandList().SetGraphicsRootSignature(mPipelineStateManager->UniversalRootSignature());
        mRingCommandList.CurrentCommandList().SetComputeRootSignature(mPipelineStateManager->UniversalRootSignature());
    }

    void GraphicsDevice::UseVertexBufferOfLayout(VertexLayout layout)
    {
        mRingCommandList.CurrentCommandList().SetVertexBuffer(*mVertexStorage->UnifiedVertexBufferDescriptorForLayout(layout));
        mRingCommandList.CurrentCommandList().SetIndexBuffer(*mVertexStorage->UnifiedIndexBufferDescriptorForLayout(layout));
        mRingCommandList.CurrentCommandList().SetPrimitiveTopology(HAL::PrimitiveTopology::TriangleList);
    }

    void GraphicsDevice::Draw(uint32_t vertexCount, uint32_t vertexStart)
    {
        mRingCommandList.CurrentCommandList().Draw(vertexCount, vertexStart);
    }

    void GraphicsDevice::DrawInstanced(uint32_t vertexCount, uint32_t vertexStart, uint32_t instanceCount)
    {
        mRingCommandList.CurrentCommandList().DrawInstanced(vertexCount, vertexStart, instanceCount);
    }

    void GraphicsDevice::DrawIndexed(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart)
    {
        mRingCommandList.CurrentCommandList().DrawIndexed(vertexStart, indexCount, indexStart);
    }

    void GraphicsDevice::DrawIndexedInstanced(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart, uint32_t instanceCount)
    {
        mRingCommandList.CurrentCommandList().DrawIndexedInstanced(vertexStart, indexCount, indexStart, instanceCount);
    }

    void GraphicsDevice::Draw(const VertexStorageLocation& vertexStorageLocation)
    {
        mRingCommandList.CurrentCommandList().DrawIndexed(vertexStorageLocation.VertexBufferOffset, vertexStorageLocation.IndexCount, vertexStorageLocation.IndexBufferOffset);
    }

    void GraphicsDevice::TransitionResource(const HAL::ResourceTransitionBarrier& barrier)
    {
        mRingCommandList.CurrentCommandList().TransitionResourceState(barrier);
    }

    void GraphicsDevice::ExecuteCommandBuffer()
    {
        
    }

}
