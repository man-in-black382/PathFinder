#include "GraphicsDevice.hpp"

namespace PathFinder
{

    GraphicsDevice::GraphicsDevice(
        const HAL::Device& device,
        const ResourceStorage* resourceManager,
        const PipelineStateManager* pipelineStateManager,
        const VertexStorage* vertexStorage,
        uint8_t simultaneousFramesInFlight)
        :
        mCommandQueue{ device },
        mRingCommandList{ device, simultaneousFramesInFlight },
        mResourceStorage{ resourceManager },
        mPipelineStateManager{ pipelineStateManager },
        mVertexStorage{ vertexStorage } {}

    void GraphicsDevice::SetRenderTarget(Foundation::Name resourceName)
    {
        mRingCommandList.CurrentCommandList().SetRenderTarget(mResourceStorage->GetRenderTarget(resourceName));
    }

    void GraphicsDevice::SetBackBufferAsRenderTarget(std::optional<Foundation::Name> depthStencilResourceName)
    {
        if (depthStencilResourceName)
        {
            mRingCommandList.CurrentCommandList().SetRenderTarget(
                mResourceStorage->GetBackBuffer(),
                mResourceStorage->GetDepthStencil(*depthStencilResourceName)
            );
        }
        else {
            mRingCommandList.CurrentCommandList().SetRenderTarget(mResourceStorage->GetBackBuffer());
        }
    }

    void GraphicsDevice::SetRenderTargetAndDepthStencil(Foundation::Name rtResourceName, Foundation::Name dsResourceName)
    {
        mRingCommandList.CurrentCommandList().SetRenderTarget(
            mResourceStorage->GetRenderTarget(rtResourceName),
            mResourceStorage->GetDepthStencil(dsResourceName)
        );
    }

    void GraphicsDevice::ClearRenderTarget(Foundation::Name resourceName, const Foundation::Color& color)
    {
        mRingCommandList.CurrentCommandList().ClearRenderTarget(mResourceStorage->GetRenderTarget(resourceName), color);
    }

    void GraphicsDevice::ClearBackBuffer(const Foundation::Color& color)
    {
        mRingCommandList.CurrentCommandList().ClearRenderTarget(mResourceStorage->GetBackBuffer(), color);
    }

    void GraphicsDevice::ClearDepth(Foundation::Name resourceName, float depthValue)
    {
        mRingCommandList.CurrentCommandList().CleadDepthStencil(mResourceStorage->GetDepthStencil(resourceName), depthValue);
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

    void GraphicsDevice::BeginFrame(uint64_t frameFenceValue)
    {
        mRingCommandList.PrepareCommandListForNewFrame(frameFenceValue);
    }

    void GraphicsDevice::EndFrame(uint64_t completedFrameFenceValue)
    {
        mRingCommandList.ReleaseAndResetForCompletedFrames(completedFrameFenceValue);
    }

    void GraphicsDevice::ExecuteCommandsThenSignalFence(HAL::Fence& fence)
    {
        mRingCommandList.CurrentCommandList().Close();
        mCommandQueue.ExecuteCommandList(mRingCommandList.CurrentCommandList());
        mCommandQueue.SignalFence(fence);
    }

}
