#include "GraphicsDevice.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    GraphicsDevice::GraphicsDevice(
        const HAL::Device& device,
        const HAL::CBSRUADescriptorHeap* universalGPUDescriptorHeap,
        PipelineResourceStorage* resourceManager,
        PipelineStateManager* pipelineStateManager,
        VertexStorage* vertexStorage,
        AssetResourceStorage* assetStorage,
        RenderSurfaceDescription defaultRenderSurface,
        uint8_t simultaneousFramesInFlight)
        :
        mCommandQueue{ device },
        mUniversalGPUDescriptorHeap{ universalGPUDescriptorHeap },
        mRingCommandList{ device, simultaneousFramesInFlight },
        mResourceStorage{ resourceManager },
        mVertexStorage{ vertexStorage },
        mAssetStorage{ assetStorage },
        mPipelineStateManager{ pipelineStateManager },
        mDefaultRenderSurface{ defaultRenderSurface } {}

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
        if (const HAL::GraphicsPipelineState* pso = mPipelineStateManager->GetGraphicsPipelineState(psoName))
        {
            if (mAppliedGraphicState != pso)
            {
                mAppliedGraphicState = pso;
                mAppliedComputeState = nullptr;
                mAppliedRayTracingState = nullptr;

                const HAL::RootSignature* signature = pso->GetRootSignature();
                CommandList().SetPipelineState(*pso);
                CommandList().SetGraphicsRootSignature(*signature);
                ReapplyCommonGraphicsResourceBindings();
            }
        }
        else if (const HAL::ComputePipelineState* pso = mPipelineStateManager->GetComputePipelineState(psoName))
        {
            if (mAppliedComputeState != pso)
            {
                mAppliedGraphicState = nullptr;
                mAppliedComputeState = pso;
                mAppliedRayTracingState = nullptr;

                const HAL::RootSignature* signature = pso->GetRootSignature();
                CommandList().SetPipelineState(*pso);
                CommandList().SetComputeRootSignature(*signature);
                ReapplyCommonComputeResourceBindings();
            }
        }
        else {
            assert_format(false, "Pipeline state ", psoName.ToString(), " does not exist.");
        }
    }

    void GraphicsDevice::UseVertexBufferOfLayout(VertexLayout layout)
    {
        CommandList().SetVertexBuffer(*mVertexStorage->UnifiedVertexBufferDescriptorForLayout(layout));
        CommandList().SetIndexBuffer(*mVertexStorage->UnifiedIndexBufferDescriptorForLayout(layout));
        CommandList().SetPrimitiveTopology(HAL::PrimitiveTopology::TriangleList);
    }
    void GraphicsDevice::SetViewport(const HAL::Viewport& viewport)
    {
        mCurrentPassViewport = viewport;
        CommandList().SetViewport(viewport);
    }

    void GraphicsDevice::Draw(uint32_t vertexCount, uint32_t vertexStart)
    {
        ApplyDefaultViewportIfNeeded();
        CommandList().Draw(vertexCount, vertexStart);
    }

    void GraphicsDevice::DrawInstanced(uint32_t vertexCount, uint32_t vertexStart, uint32_t instanceCount)
    {
        ApplyDefaultViewportIfNeeded();
        CommandList().DrawInstanced(vertexCount, vertexStart, instanceCount);
    }

    void GraphicsDevice::DrawIndexed(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart)
    {
        ApplyDefaultViewportIfNeeded();
        CommandList().DrawIndexed(vertexStart, indexCount, indexStart);
    }

    void GraphicsDevice::DrawIndexedInstanced(uint32_t vertexStart, uint32_t indexCount, uint32_t indexStart, uint32_t instanceCount)
    {
        ApplyDefaultViewportIfNeeded();
        CommandList().DrawIndexedInstanced(vertexStart, indexCount, indexStart, instanceCount);
    }

    void GraphicsDevice::Draw(const VertexStorageLocation& vertexStorageLocation)
    {
        ApplyDefaultViewportIfNeeded();
        CommandList().DrawIndexed(vertexStorageLocation.VertexBufferOffset, vertexStorageLocation.IndexCount, vertexStorageLocation.IndexBufferOffset);
    }
    
    void GraphicsDevice::Draw(const DrawablePrimitive& primitive)
    {
        ApplyDefaultViewportIfNeeded();
        CommandList().SetPrimitiveTopology(primitive.Topology());
        CommandList().Draw(primitive.VertexCount(), 0);
    }

    void GraphicsDevice::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        CommandList().Dispatch(groupCountX, groupCountY, groupCountZ);
    }

    void GraphicsDevice::BindMeshInstanceTableStructuredBuffer(uint16_t shaderRegister, uint16_t registerSpace)
    {
        if (mAppliedGraphicState)
        {
            auto index = mAppliedGraphicState->GetRootSignature()->GetParameterIndex({ shaderRegister, registerSpace, HAL::ShaderRegister::ShaderResource });
            assert_format(index, "Root signature parameter doesn't exist");
            CommandList().SetGraphicsRootShaderResource(mAssetStorage->InstanceTable(), *index);
        }
        else if (mAppliedComputeState)
        {
            auto index = mAppliedComputeState->GetRootSignature()->GetParameterIndex({ shaderRegister, registerSpace, HAL::ShaderRegister::ShaderResource });
            assert_format(index, "Root signature parameter doesn't exist");
            CommandList().SetGraphicsRootShaderResource(mAssetStorage->InstanceTable(), *index);
        }
        else if (mAppliedRayTracingState)
        {
            auto index = mAppliedRayTracingState->GetGlobalRootSignature()->GetParameterIndex({ shaderRegister, registerSpace, HAL::ShaderRegister::ShaderResource });
            assert_format(index, "Root signature parameter doesn't exist");
            CommandList().SetGraphicsRootShaderResource(mAssetStorage->InstanceTable(), *index);
        }
        else {
            assert_format("No PSO/Root Signature applied");
        }        
    }

    void GraphicsDevice::BindSceneRayTracingAccelerationStructure(uint16_t shaderRegister, uint16_t registerSpace)
    {
        
    }

    void GraphicsDevice::BindUnifiedVertexBufferOfLayout(VertexLayout layout, uint16_t shaderRegister, uint16_t registerSpace)
    {
        
    }

    void GraphicsDevice::BindUnifiedIndexBufferForVertexLayout(VertexLayout layout, uint16_t shaderRegister, uint16_t registerSpace)
    {
        
    }

    void GraphicsDevice::WaitFence(HAL::Fence& fence)
    {
        mCommandQueue.WaitFence(fence);
    }

    void GraphicsDevice::ExecuteCommands()
    {
        CommandList().Close();
        mCommandQueue.ExecuteCommandList(CommandList());
    }

    void GraphicsDevice::SignalFence(HAL::Fence& fence)
    {
        mCommandQueue.SignalFence(fence);
    }

    void GraphicsDevice::BeginFrame(uint64_t frameFenceValue)
    {
        mRingCommandList.PrepareCommandListForNewFrame(frameFenceValue);
    }

    void GraphicsDevice::EndFrame(uint64_t completedFrameFenceValue)
    {
        mRingCommandList.ReleaseAndResetForCompletedFrames(completedFrameFenceValue);
    }

    void GraphicsDevice::SetCurrentRenderPass(const RenderPass* pass)
    {
        mCurrentRenderPass = pass;
        mCurrentPassViewport = std::nullopt;
    }

    void GraphicsDevice::ReapplyCommonGraphicsResourceBindings()
    {   
        CommandList().SetDescriptorHeap(*mUniversalGPUDescriptorHeap);

        // Look at PipelineStateManager for base root signature parameter ordering
        CommandList().SetGraphicsRootConstantBuffer(mResourceStorage->GlobalRootConstantsBuffer(), 0);
        CommandList().SetGraphicsRootConstantBuffer(mResourceStorage->PerFrameRootConstantsBuffer(), 1);

        if (auto buffer = mResourceStorage->RootConstantBufferForCurrentPass()) CommandList().SetGraphicsRootConstantBuffer(*buffer, 2);

        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture2D, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 3);
        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture3D, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 4);
        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture2DArray, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 5);
        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture2D, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 6);
        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture3D, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 7);
        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture2DArray, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 8);
    }

    void GraphicsDevice::ReapplyCommonComputeResourceBindings()
    {
        CommandList().SetDescriptorHeap(*mUniversalGPUDescriptorHeap);

        // Look at PipelineStateManager for base root signature parameter ordering
        CommandList().SetComputeRootConstantBuffer(mResourceStorage->GlobalRootConstantsBuffer(), 0);
        CommandList().SetComputeRootConstantBuffer(mResourceStorage->PerFrameRootConstantsBuffer(), 1);

        if (auto buffer = mResourceStorage->RootConstantBufferForCurrentPass()) CommandList().SetComputeRootConstantBuffer(*buffer, 2);

        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture2D, 0)) CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 3);
        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture3D, 0)) CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 4);
        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture2DArray, 0)) CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 5);
        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture2D, 0)) CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 6);
        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture3D, 0)) CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 7);
        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture2DArray, 0)) CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 8);
    }

    void GraphicsDevice::ApplyDefaultViewportIfNeeded()
    {
        if (mCurrentPassViewport) return;
           
        mCurrentPassViewport = HAL::Viewport(mDefaultRenderSurface.Dimensions().Width, mDefaultRenderSurface.Dimensions().Height);
        CommandList().SetViewport(*mCurrentPassViewport);
    }

}
