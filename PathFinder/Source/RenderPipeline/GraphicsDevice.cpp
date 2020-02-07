#include "GraphicsDevice.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    GraphicsDevice::GraphicsDevice(
        const HAL::Device& device, 
        const HAL::CBSRUADescriptorHeap* universalGPUDescriptorHeap,
        Memory::PoolCommandListAllocator* commandListAllocator,
        PipelineResourceStorage* resourceStorage,
        PipelineStateManager* pipelineStateManager,
        const RenderSurfaceDescription& defaultRenderSurface)
        :
        GraphicsDeviceBase(
            device, 
            universalGPUDescriptorHeap, 
            commandListAllocator, 
            resourceStorage, 
            pipelineStateManager, 
            defaultRenderSurface)
    {
        mCommandQueue.SetDebugName("Graphics Device Command Queue");
    }

    void GraphicsDevice::SetRenderTarget(Foundation::Name resourceName)
    {
        mCommandList->SetRenderTarget(mResourceStorage->GetRenderTargetDescriptor(resourceName));
    }

    void GraphicsDevice::SetBackBufferAsRenderTarget(std::optional<Foundation::Name> depthStencilResourceName)
    {
        if (depthStencilResourceName)
        {
            mCommandList->SetRenderTarget(
                mResourceStorage->GetCurrentBackBufferDescriptor(),
                mResourceStorage->GetDepthStencilDescriptor(*depthStencilResourceName)
            );
        }
        else {
            mCommandList->SetRenderTarget(mResourceStorage->GetCurrentBackBufferDescriptor());
        }
    }

    void GraphicsDevice::SetRenderTargetAndDepthStencil(Foundation::Name rtResourceName, Foundation::Name dsResourceName)
    {
        mCommandList->SetRenderTarget(
            mResourceStorage->GetRenderTargetDescriptor(rtResourceName),
            mResourceStorage->GetDepthStencilDescriptor(dsResourceName)
        );
    }

    void GraphicsDevice::ClearRenderTarget(Foundation::Name resourceName, const Foundation::Color& color)
    {
        mCommandList->ClearRenderTarget(mResourceStorage->GetRenderTargetDescriptor(resourceName), color);
    }

    void GraphicsDevice::ClearBackBuffer(const Foundation::Color& color)
    {
        mCommandList->ClearRenderTarget(mResourceStorage->GetCurrentBackBufferDescriptor(), color);
    }

    void GraphicsDevice::ClearDepth(Foundation::Name resourceName, float depthValue)
    {
        mCommandList->CleadDepthStencil(mResourceStorage->GetDepthStencilDescriptor(resourceName), depthValue);
    }

    void GraphicsDevice::SetViewport(const HAL::Viewport& viewport)
    {
        mCurrentPassViewport = viewport;
        mCommandList->SetViewport(viewport);
    }

    void GraphicsDevice::Draw(uint32_t vertexCount, uint32_t instanceCount)
    {
        ApplyDefaultViewportIfNeeded();
        mCommandList->Draw(vertexCount, 0);
        mCommandList->InsertBarriers(mResourceStorage->UnorderedAccessBarriersForCurrentPass());
    }
    
    void GraphicsDevice::Draw(const DrawablePrimitive& primitive)
    {
        ApplyDefaultViewportIfNeeded();
        mCommandList->SetPrimitiveTopology(primitive.Topology());
        mCommandList->Draw(primitive.VertexCount(), 0);
        mCommandList->InsertBarriers(mResourceStorage->UnorderedAccessBarriersForCurrentPass());
    }

    void GraphicsDevice::ResetViewportToDefault()
    {
        mCurrentPassViewport = std::nullopt;
    }

    void GraphicsDevice::ApplyCommonGraphicsResourceBindings()
    {   
        mCommandList->SetDescriptorHeap(*mUniversalGPUDescriptorHeap);

        // Look at PipelineStateManager for base root signature parameter ordering
        //mCommandList->SetGraphicsRootConstantBuffer(mResourceStorage->GlobalRootConstantsBuffer(), 0);
        //mCommandList->SetGraphicsRootConstantBuffer(mResourceStorage->PerFrameRootConstantsBuffer(), 1);

        HAL::DescriptorAddress SRRangeAddress = mUniversalGPUDescriptorHeap->RangeStartGPUAddress(HAL::CBSRUADescriptorHeap::Range::ShaderResource);
        HAL::DescriptorAddress UARangeAddress = mUniversalGPUDescriptorHeap->RangeStartGPUAddress(HAL::CBSRUADescriptorHeap::Range::UnorderedAccess);

        // Alias different registers to one GPU address
        mCommandList->SetGraphicsRootDescriptorTable(SRRangeAddress, 3);
        mCommandList->SetGraphicsRootDescriptorTable(SRRangeAddress, 4);
        mCommandList->SetGraphicsRootDescriptorTable(SRRangeAddress, 5);
        mCommandList->SetGraphicsRootDescriptorTable(SRRangeAddress, 6);
        mCommandList->SetGraphicsRootDescriptorTable(SRRangeAddress, 7);

        mCommandList->SetGraphicsRootDescriptorTable(UARangeAddress, 8);
        mCommandList->SetGraphicsRootDescriptorTable(UARangeAddress, 9);
        mCommandList->SetGraphicsRootDescriptorTable(UARangeAddress, 10);
        mCommandList->SetGraphicsRootDescriptorTable(UARangeAddress, 11);
        mCommandList->SetGraphicsRootDescriptorTable(UARangeAddress, 12);
    }

    void GraphicsDevice::BindCurrentPassBuffersGraphics()
    {
       /* if (auto buffer = mResourceStorage->RootConstantBufferForCurrentPass())
        {
            mCommandList->SetGraphicsRootConstantBuffer(*buffer, 2);
        }*/

        //mCommandList->SetGraphicsRootUnorderedAccessResource(*mResourceStorage->DebugBufferForCurrentPass(), 13);
    }

    void GraphicsDevice::ApplyDefaultViewportIfNeeded()
    {
        if (mCurrentPassViewport) return;

        mCurrentPassViewport = HAL::Viewport(mDefaultRenderSurface.Dimensions().Width, mDefaultRenderSurface.Dimensions().Height);
        mCommandList->SetViewport(*mCurrentPassViewport);
    }

    void GraphicsDevice::ApplyPipelineState(Foundation::Name psoName)
    {
        const PipelineStateManager::PipelineStateVariant* state = mPipelineStateManager->GetPipelineState(psoName);
        assert_format(state, "Pipeline state doesn't exist");

        if (std::holds_alternative<HAL::ComputePipelineState>(*state))
        {
            ApplyStateIfNeeded(&std::get<HAL::ComputePipelineState>(*state));
        }
        else if (std::holds_alternative<HAL::RayTracingPipelineState>(*state))
        {
            ApplyStateIfNeeded(&std::get<HAL::RayTracingPipelineState>(*state));
        }
        else if (std::holds_alternative<HAL::GraphicsPipelineState>(*state))
        {
            ApplyStateIfNeeded(&std::get<HAL::GraphicsPipelineState>(*state));
        }
    }

    void GraphicsDevice::ApplyStateIfNeeded(const HAL::GraphicsPipelineState* state)
    {
        bool graphicsStateApplied = mAppliedGraphicsState != nullptr;

        // State is already applied
        //
        if (graphicsStateApplied && mAppliedGraphicsState == state) return;

        // Set RT state
        mCommandList->SetPipelineState(*state);
        mCommandList->SetPrimitiveTopology(state->GetPrimitiveTopology());

        // Same root signature is already applied as a graphics signature
        //
        if (graphicsStateApplied && state->GetRootSignature() == mAppliedGraphicsRootSignature)
        {
            BindCurrentPassBuffersGraphics();
            return;
        }

        mCommandList->SetGraphicsRootSignature(*state->GetRootSignature());
        ApplyCommonGraphicsResourceBindings();
        BindCurrentPassBuffersGraphics();

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
        //mCommandList->SetPipelineState(*pso);

        // Same root signature is already applied as a graphics signature
        //
        if (rayTracingStateApplied && state->GetGlobalRootSignature() == mAppliedGraphicsRootSignature)
        {
            BindCurrentPassBuffersGraphics();
            return;
        }

        mCommandList->SetGraphicsRootSignature(*state->GetGlobalRootSignature());
        ApplyCommonGraphicsResourceBindings();
        BindCurrentPassBuffersGraphics();

        // Nullify cached states and signatures
        mAppliedGraphicsState = nullptr;
        mAppliedComputeState = nullptr;
        mAppliedComputeRootSignature = nullptr;
        
        // Cache RT state and signature as graphics signature
        mAppliedRayTracingState = state;
        mAppliedGraphicsRootSignature = state->GetGlobalRootSignature();
    }
    
    void GraphicsDevice::BindExternalBuffer(const Memory::Buffer& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        if (mAppliedComputeState)
        {
            GraphicsDeviceBase::BindExternalBuffer(buffer, shaderRegister, registerSpace, registerType);
        }
        else if (mAppliedGraphicsState || mAppliedRayTracingState)
        {
            const HAL::RootSignature* signature = mAppliedGraphicsState ?
                mAppliedGraphicsState->GetRootSignature() : mAppliedRayTracingState->GetGlobalRootSignature();

            auto index = signature->GetParameterIndex({ shaderRegister, registerSpace, registerType });

            assert_format(index, "Root signature parameter doesn't exist");

            // Will be changed if I'll see render passes that require a lot of buffers. 
            assert_format(!index->IsIndirect, "Descriptor tables for buffers are not supported. Bind buffers directly instead.");

            switch (registerType)
            {
            case HAL::ShaderRegister::ShaderResource: mCommandList->SetGraphicsRootShaderResource(*buffer.HALBuffer(), index->IndexInSignature); break;
            case HAL::ShaderRegister::ConstantBuffer: mCommandList->SetGraphicsRootConstantBuffer(*buffer.HALBuffer(), index->IndexInSignature); break;
            case HAL::ShaderRegister::UnorderedAccess: mCommandList->SetGraphicsRootUnorderedAccessResource(*buffer.HALBuffer(), index->IndexInSignature); break;
            case HAL::ShaderRegister::Sampler:
                assert_format(false, "Incompatible register type");
            }
        }
        else {
            assert_format(false, "No pipeline state applied");
        }
    }

}
