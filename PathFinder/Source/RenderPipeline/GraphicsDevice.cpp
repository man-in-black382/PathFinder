#include "GraphicsDevice.hpp"

namespace PathFinder
{

    GraphicsDevice::GraphicsDevice(
        const HAL::Device& device,
        ResourceStorage* resourceManager,
        PipelineStateManager* pipelineStateManager,
        VertexStorage* vertexStorage,
        uint8_t simultaneousFramesInFlight)
        :
        mCommandQueue{ device },
        mRingCommandList{ device, simultaneousFramesInFlight },
        mResourceStorage{ resourceManager },
        mPipelineStateManager{ pipelineStateManager },
        mVertexStorage{ vertexStorage } {}

    void GraphicsDevice::SetRenderTarget(Foundation::Name resourceName)
    {
        CommandList().SetRenderTarget(mResourceStorage->GetRenderTargetDescriptor(resourceName));
    }

    void GraphicsDevice::SetBackBufferAsRenderTarget(std::optional<Foundation::Name> depthStencilResourceName)
    {
        if (depthStencilResourceName)
        {
            CommandList().SetRenderTarget(
                mResourceStorage->GetCurrentBackBufferDescriptor(),
                mResourceStorage->GetDepthStencilDescriptor(*depthStencilResourceName)
            );
        }
        else {
            CommandList().SetRenderTarget(mResourceStorage->GetCurrentBackBufferDescriptor());
        }
    }

    void GraphicsDevice::SetRenderTargetAndDepthStencil(Foundation::Name rtResourceName, Foundation::Name dsResourceName)
    {
        CommandList().SetRenderTarget(
            mResourceStorage->GetRenderTargetDescriptor(rtResourceName),
            mResourceStorage->GetDepthStencilDescriptor(dsResourceName)
        );
    }

    void GraphicsDevice::ClearRenderTarget(Foundation::Name resourceName, const Foundation::Color& color)
    {
        CommandList().ClearRenderTarget(mResourceStorage->GetRenderTargetDescriptor(resourceName), color);
    }

    void GraphicsDevice::ClearBackBuffer(const Foundation::Color& color)
    {
        CommandList().ClearRenderTarget(mResourceStorage->GetCurrentBackBufferDescriptor(), color);
    }

    void GraphicsDevice::ClearDepth(Foundation::Name resourceName, float depthValue)
    {
        CommandList().CleadDepthStencil(mResourceStorage->GetDepthStencilDescriptor(resourceName), depthValue);
    }

    void GraphicsDevice::ApplyPipelineState(Foundation::Name psoName)
    {
        auto& pso = mPipelineStateManager->GetPipelineState(psoName);

        CommandList().SetPipelineState(pso);
        CommandList().SetGraphicsRootSignature(*pso.GetRootSignature());
        CommandList().SetComputeRootSignature(*pso.GetRootSignature());

        ReapplyCommonResourceBindings();
    }

    void GraphicsDevice::UseVertexBufferOfLayout(VertexLayout layout)
    {
        CommandList().SetVertexBuffer(*mVertexStorage->UnifiedVertexBufferDescriptorForLayout(layout));
        CommandList().SetIndexBuffer(*mVertexStorage->UnifiedIndexBufferDescriptorForLayout(layout));
        CommandList().SetPrimitiveTopology(HAL::PrimitiveTopology::TriangleList);
    }
    void GraphicsDevice::SetViewport(const HAL::Viewport& viewport)
    {
        CommandList().SetViewport(viewport);
    }

    void GraphicsDevice::Draw(uint32_t vertexCount, uint32_t vertexStart)
    {
        CommandList().Draw(vertexCount, vertexStart);
    }

    void GraphicsDevice::DrawInstanced(uint32_t vertexCount, uint32_t vertexStart, uint32_t instanceCount)
    {
        CommandList().DrawInstanced(vertexCount, vertexStart, instanceCount);
    }

    void GraphicsDevice::DrawIndexed(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart)
    {
        CommandList().DrawIndexed(vertexStart, indexCount, indexStart);
    }

    void GraphicsDevice::DrawIndexedInstanced(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart, uint32_t instanceCount)
    {
        CommandList().DrawIndexedInstanced(vertexStart, indexCount, indexStart, instanceCount);
    }

    void GraphicsDevice::Draw(const VertexStorageLocation& vertexStorageLocation)
    {
        CommandList().DrawIndexed(vertexStorageLocation.VertexBufferOffset, vertexStorageLocation.IndexCount, vertexStorageLocation.IndexBufferOffset);
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
        CommandList().Close();
        mCommandQueue.ExecuteCommandList(CommandList());
        mCommandQueue.SignalFence(fence);
    }

    void GraphicsDevice::ReapplyCommonResourceBindings()
    {
        const HAL::CBSRUADescriptorHeap& heap = mResourceStorage->DescriptorStorage().CBSRUADescriptorHeap();
        
        // Look at PipelineStateManager for base root signature parameter ordering

        if (auto baseDescriptor = heap.GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture2D, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 0);
        if (auto baseDescriptor = heap.GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture3D, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 1);
        if (auto baseDescriptor = heap.GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture2DArray, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 2);
        if (auto baseDescriptor = heap.GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture2D, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 3);
        if (auto baseDescriptor = heap.GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture3D, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 4);
        if (auto baseDescriptor = heap.GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture2DArray, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 5);

        CommandList().SetGraphicsRootConstantBuffer(mResourceStorage->GlobalRootConstantsBuffer(), 6);
        CommandList().SetGraphicsRootConstantBuffer(mResourceStorage->PerFrameRootConstantsBuffer(), 7);

        if (auto buffer = mResourceStorage->RootConstantBufferForCurrentPass()) CommandList().SetGraphicsRootConstantBuffer(*buffer, 8);         
    }

}
