#include "GraphicsDevice.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

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

    void GraphicsDevice::SetViewport(const HAL::Viewport& viewport)
    {
        mCurrentPassViewport = viewport;
        CommandList().SetViewport(viewport);
    }

    void GraphicsDevice::Draw(uint32_t vertexCount, uint32_t instanceCount)
    {
        ApplyDefaultViewportIfNeeded();
        CommandList().Draw(vertexCount, 0);
    }
    
    void GraphicsDevice::Draw(const DrawablePrimitive& primitive)
    {
        ApplyDefaultViewportIfNeeded();
        CommandList().SetPrimitiveTopology(primitive.Topology());
        CommandList().Draw(primitive.VertexCount(), 0);
    }

    void GraphicsDevice::ResetViewportToDefault()
    {
        mCurrentPassViewport = std::nullopt;
    }

    void GraphicsDevice::ApplyCommonGraphicsResourceBindings()
    {   
        CommandList().SetDescriptorHeap(*mUniversalGPUDescriptorHeap);

        // Look at PipelineStateManager for base root signature parameter ordering
        CommandList().SetGraphicsRootConstantBuffer(mResourceStorage->GlobalRootConstantsBuffer(), 0);
        CommandList().SetGraphicsRootConstantBuffer(mResourceStorage->PerFrameRootConstantsBuffer(), 1);

        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture2D, 0))
        {
            CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 3);
            CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 4);
        }
            
        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture3D, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 5);
        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture2DArray, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 6);
       
        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture2D, 0))
        {
            CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 7);
            CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 8);
        }
            
        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture3D, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 9);
        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture2DArray, 0)) CommandList().SetGraphicsRootDescriptorTable(*baseDescriptor, 10);
    }

    void GraphicsDevice::BindCurrentPassConstantBufferGraphics()
    {
        if (auto buffer = mResourceStorage->RootConstantBufferForCurrentPass())
        {
            CommandList().SetGraphicsRootConstantBuffer(*buffer, 2);
        }
    }

    void GraphicsDevice::ApplyDefaultViewportIfNeeded()
    {
        if (mCurrentPassViewport) return;

        mCurrentPassViewport = HAL::Viewport(mDefaultRenderSurface.Dimensions().Width, mDefaultRenderSurface.Dimensions().Height);
        CommandList().SetViewport(*mCurrentPassViewport);
    }

    void GraphicsDevice::ApplyPipelineState(Foundation::Name psoName)
    {
        if (const HAL::GraphicsPipelineState* pso = this->mPipelineStateManager->GetGraphicsPipelineState(psoName))
        {
            ApplyStateIfNeeded(pso);
        }
        else if (const HAL::ComputePipelineState* pso = this->mPipelineStateManager->GetComputePipelineState(psoName))
        {
            ApplyStateIfNeeded(pso);
        }
        else if (const HAL::RayTracingPipelineState* pso = this->mPipelineStateManager->GetRayTracingPipelineState(psoName))
        {
            ApplyStateIfNeeded(pso);
        }
        else {
            assert_format(false, "Pipeline state doesn't exist");
        }
    }

    void GraphicsDevice::ApplyStateIfNeeded(const HAL::GraphicsPipelineState* state)
    {
        bool graphicsStateApplied = mAppliedGraphicsState != nullptr;

        // State is already applied
        //
        if (graphicsStateApplied && mAppliedGraphicsState == state) return;

        // Set RT state
        CommandList().SetPipelineState(*state);
        CommandList().SetPrimitiveTopology(state->GetPrimitiveTopology());

        // Same root signature is already applied as a graphics signature
        //
        if (graphicsStateApplied && state->GetRootSignature() == mAppliedGraphicsRootSignature)
        {
            BindCurrentPassConstantBufferGraphics();
            return;
        }

        CommandList().SetGraphicsRootSignature(*state->GetRootSignature());
        ApplyCommonGraphicsResourceBindings();
        BindCurrentPassConstantBufferGraphics();

        // Nullify cached states and signatures
        mAppliedComputeState = nullptr;
        mAppliedRayTracingState = nullptr;
        mAppliedComputeRootSignature = nullptr;

        // Cache graphics state and signature as graphics signature
        mAppliedGraphicsState = state;
        mAppliedGraphicsRootSignature = state->GetRootSignature();
    }

    void GraphicsDevice::ApplyStateIfNeeded(const HAL::ComputePipelineState* state)
    {
        // Since Graphics and Compute devices share compute functionality
        // we can reuse base class implementation
        GraphicsDeviceBase::ApplyStateIfNeeded(state);

        // When compute state is applied we need to clear graphics state/signature cache
        mAppliedGraphicsRootSignature = nullptr;
        mAppliedGraphicsState = nullptr;
    }

    void GraphicsDevice::ApplyStateIfNeeded(const HAL::RayTracingPipelineState* state)
    {
        // Ray tracing on graphics queue is a graphics workload, therefore we 
        // need to override base functionality that sees RT work as compute work

        bool rayTracingStateApplied = mAppliedRayTracingState != nullptr;

        // State is already applied
        //
        if (rayTracingStateApplied && mAppliedRayTracingState == state) return;

        // Set RT state
        //CommandList().SetPipelineState(*pso);

        // Same root signature is already applied as a graphics signature
        //
        if (rayTracingStateApplied && state->GetGlobalRootSignature() == mAppliedGraphicsRootSignature)
        {
            BindCurrentPassConstantBufferGraphics();
            return;
        }

        CommandList().SetGraphicsRootSignature(*state->GetGlobalRootSignature());
        ApplyCommonGraphicsResourceBindings();
        BindCurrentPassConstantBufferGraphics();

        // Nullify cached states and signatures
        mAppliedGraphicsState = nullptr;
        mAppliedComputeState = nullptr;
        mAppliedComputeRootSignature = nullptr;
        
        // Cache RT state and signature as graphics signature
        mAppliedRayTracingState = state;
        mAppliedGraphicsRootSignature = state->GetGlobalRootSignature();
    }

}
