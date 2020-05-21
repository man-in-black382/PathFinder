#include "GraphicsDevice.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    GraphicsDevice::GraphicsDevice(
        const HAL::Device& device, 
        const HAL::CBSRUADescriptorHeap* universalGPUDescriptorHeap,
        Memory::PoolCommandListAllocator* commandListAllocator,
        Memory::ResourceStateTracker* resourceStateTracker,
        PipelineResourceStorage* resourceStorage,
        PipelineStateManager* pipelineStateManager,
        const RenderSurfaceDescription& defaultRenderSurface)
        :
        GraphicsDeviceBase(
            device, 
            universalGPUDescriptorHeap, 
            commandListAllocator, 
            resourceStateTracker,
            resourceStorage, 
            pipelineStateManager, 
            defaultRenderSurface)
    {
        mCommandQueue.SetDebugName("Graphics Device Command Queue");
    }

    void GraphicsDevice::SetRenderTarget(const ResourceKey& rtKey, std::optional<ResourceKey> dsKey)
    {
        const HAL::DSDescriptor* dsDescriptor = dsKey ? mResourceStorage->GetDepthStencilDescriptor(dsKey->ResourceName(), dsKey->IndexInArray()) : nullptr;
        mCommandList->SetRenderTarget(*mResourceStorage->GetRenderTargetDescriptor(rtKey.ResourceName(), rtKey.IndexInArray()), dsDescriptor);
    }

    void GraphicsDevice::SetBackBufferAsRenderTarget(std::optional<ResourceKey> dsKey)
    {
        const HAL::DSDescriptor* dsDescriptor = dsKey ? mResourceStorage->GetDepthStencilDescriptor(dsKey->ResourceName(), dsKey->IndexInArray()) : nullptr;
        mCommandList->SetRenderTarget(*mBackBuffer->GetRTDescriptor(), dsDescriptor);
    }

    void GraphicsDevice::ClearRenderTarget(const ResourceKey& rtKey)
    {
        // Clear operation requires correct states same as a dispatch or a draw
        InsertResourceTransitionsIfNeeded();

        const Memory::Texture* renderTarget = mResourceStorage->GetPerResourceData(rtKey.ResourceName())->GetTexture(rtKey.IndexInArray());
        assert_format(renderTarget, "Render target doesn't exist");
        auto clearValue = std::get_if<HAL::ColorClearValue>(&renderTarget->Properties().OptimizedClearValue);
        assert_format(clearValue, "Texture does not contain optimized color clear value");

        mCommandList->ClearRenderTarget(*mResourceStorage->GetRenderTargetDescriptor(rtKey.ResourceName(), rtKey.IndexInArray()), *clearValue);
    }

    void GraphicsDevice::ClearDepth(const ResourceKey& dsKey)
    {
        // Clear operation requires correct states same as a dispatch or a draw
        InsertResourceTransitionsIfNeeded();

        const Memory::Texture* depthAttachment = mResourceStorage->GetPerResourceData(dsKey.ResourceName())->GetTexture(dsKey.IndexInArray());
        assert_format(depthAttachment, "Depth/stencil attachment doesn't exist");
        auto clearValue = std::get_if<HAL::DepthStencilClearValue>(&depthAttachment->Properties().OptimizedClearValue);
        assert_format(clearValue, "Texture does not contain optimized depth/stencil clear value");

        mCommandList->CleadDepthStencil(*mResourceStorage->GetDepthStencilDescriptor(dsKey.ResourceName(), dsKey.IndexInArray()), clearValue->Depth);
    }

    void GraphicsDevice::SetViewport(const HAL::Viewport& viewport)
    {
        mCurrentPassViewport = viewport;
        mCommandList->SetViewport(viewport);
    }

    void GraphicsDevice::Draw(uint32_t vertexCount, uint32_t instanceCount)
    {
        ApplyCommonGraphicsResourceBindingsIfNeeded();
        ApplyDefaultViewportIfNeeded();
        InsertResourceTransitionsIfNeeded();

        mCommandList->Draw(vertexCount, 0);
        mResourceStorage->AllowCurrentPassConstantBufferSingleOffsetAdvancement();
    }
    
    void GraphicsDevice::Draw(const DrawablePrimitive& primitive)
    {
        ApplyCommonGraphicsResourceBindingsIfNeeded();
        ApplyDefaultViewportIfNeeded();
        InsertResourceTransitionsIfNeeded();

        mCommandList->SetPrimitiveTopology(primitive.Topology());
        mCommandList->Draw(primitive.VertexCount(), 0);
        mResourceStorage->AllowCurrentPassConstantBufferSingleOffsetAdvancement();
    }

    void GraphicsDevice::ResetViewportToDefault()
    {
        mCurrentPassViewport = std::nullopt;
    }

    void GraphicsDevice::ApplyCommonGraphicsResourceBindingsIfNeeded()
    {   
        auto commonParametersIndexOffset = mAppliedGraphicsRootSignature->ParameterCount() - mPipelineStateManager->CommonRootSignatureParameterCount();

        if (mRebindingAfterSignatureChangeRequired)
        {
            // Look at PipelineStateManager for base root signature parameter ordering
            HAL::DescriptorAddress SRRangeAddress = mUniversalGPUDescriptorHeap->RangeStartGPUAddress(HAL::CBSRUADescriptorHeap::Range::ShaderResource);
            HAL::DescriptorAddress UARangeAddress = mUniversalGPUDescriptorHeap->RangeStartGPUAddress(HAL::CBSRUADescriptorHeap::Range::UnorderedAccess);

            // Alias different registers to one GPU address
            mCommandList->SetGraphicsRootDescriptorTable(SRRangeAddress, 3 + commonParametersIndexOffset);
            mCommandList->SetGraphicsRootDescriptorTable(SRRangeAddress, 4 + commonParametersIndexOffset);
            mCommandList->SetGraphicsRootDescriptorTable(SRRangeAddress, 5 + commonParametersIndexOffset);
            mCommandList->SetGraphicsRootDescriptorTable(SRRangeAddress, 6 + commonParametersIndexOffset);
            mCommandList->SetGraphicsRootDescriptorTable(SRRangeAddress, 7 + commonParametersIndexOffset);

            mCommandList->SetGraphicsRootDescriptorTable(UARangeAddress, 8 + commonParametersIndexOffset);
            mCommandList->SetGraphicsRootDescriptorTable(UARangeAddress, 9 + commonParametersIndexOffset);
            mCommandList->SetGraphicsRootDescriptorTable(UARangeAddress, 10 + commonParametersIndexOffset);
            mCommandList->SetGraphicsRootDescriptorTable(UARangeAddress, 11 + commonParametersIndexOffset);
            mCommandList->SetGraphicsRootDescriptorTable(UARangeAddress, 12 + commonParametersIndexOffset);
        }

        if (auto buffer = mResourceStorage->GlobalRootConstantsBuffer();
            buffer && (buffer->HALBuffer() != mBoundGlobalConstantBufferGraphics || mRebindingAfterSignatureChangeRequired))
        {
            mBoundGlobalConstantBufferGraphics = buffer->HALBuffer();
            mCommandList->SetGraphicsRootConstantBuffer(*mBoundGlobalConstantBufferGraphics, 0 + commonParametersIndexOffset);
        }

        if (auto buffer = mResourceStorage->PerFrameRootConstantsBuffer();
            buffer && (buffer->HALBuffer() != mBoundFrameConstantBufferGraphics || mRebindingAfterSignatureChangeRequired))
        {
            mBoundFrameConstantBufferGraphics = buffer->HALBuffer();
            mCommandList->SetGraphicsRootConstantBuffer(*mBoundFrameConstantBufferGraphics, 1 + commonParametersIndexOffset);
        }

        if (auto address = mResourceStorage->RootConstantsBufferAddressForCurrentPass();
            address != mBoundFrameConstantBufferAddressGraphics || mRebindingAfterSignatureChangeRequired)
        {
            mBoundFrameConstantBufferAddressGraphics = address;
            mCommandList->SetGraphicsRootConstantBuffer(address, 2 + commonParametersIndexOffset);
        }

        if (auto buffer = mResourceStorage->DebugBufferForCurrentPass();
            buffer && (buffer->HALBuffer() != mBoundPassDebugBufferGraphics || mRebindingAfterSignatureChangeRequired))
        {
            mBoundPassDebugBufferGraphics = buffer->HALBuffer();
            mCommandList->SetGraphicsRootUnorderedAccessResource(*mBoundPassDebugBufferGraphics, 13 + commonParametersIndexOffset);
        }

        mRebindingAfterSignatureChangeRequired = false;
    }

    void GraphicsDevice::ApplyDefaultViewportIfNeeded()
    {
        if (mCurrentPassViewport) return;

        mCurrentPassViewport = HAL::Viewport(mDefaultRenderSurface.Dimensions().Width, mDefaultRenderSurface.Dimensions().Height);
        mCommandList->SetViewport(*mCurrentPassViewport);
    }

    void GraphicsDevice::ApplyPipelineState(Foundation::Name psoName)
    {
        std::optional<PipelineStateManager::PipelineStateVariant> state = mPipelineStateManager->GetPipelineState(psoName);
        assert_format(state, "Pipeline state doesn't exist");

        if (state->ComputePSO) ApplyStateIfNeeded(state->ComputePSO);
        else if (state->GraphicPSO) ApplyStateIfNeeded(state->GraphicPSO);
        else if (state->RayTracingPSO) ApplyStateIfNeeded(state->RayTracingPSO, state->BaseRayDispatchInfo);
    }

    void GraphicsDevice::ApplyStateIfNeeded(const HAL::GraphicsPipelineState* state)
    {
        bool graphicsStateApplied = mAppliedGraphicsState != nullptr;

        // State is already applied
        //
        if (graphicsStateApplied && mAppliedGraphicsState == state) return;

        // Set Graphics state
        mCommandList->SetPipelineState(*state);
        mCommandList->SetPrimitiveTopology(state->GetPrimitiveTopology());

        mRebindingAfterSignatureChangeRequired = state->GetRootSignature() != mAppliedGraphicsRootSignature;

        // If root signature has changed we need to rebind common bindings
        //
        if (mRebindingAfterSignatureChangeRequired)
        {
            mCommandList->SetGraphicsRootSignature(*state->GetRootSignature());
        }

        // Nullify cached states and signatures
        mAppliedComputeState = nullptr;
        mAppliedRayTracingState = nullptr;
        mAppliedRayTracingDispatchInfo = nullptr;
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

    void GraphicsDevice::ApplyStateIfNeeded(const HAL::RayTracingPipelineState* state, const HAL::RayDispatchInfo* dispatchInfo)
    {
        // Ray Tracing is a compute workload:
        // reuse base class implementation
        GraphicsDeviceBase::ApplyStateIfNeeded(state, dispatchInfo);

        // When RT state is applied we need to clear graphics state/signature cache
        mAppliedGraphicsRootSignature = nullptr;
        mAppliedGraphicsState = nullptr;
    }
    
    void GraphicsDevice::BindExternalBuffer(const Memory::Buffer& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        // Ray Tracing bindings go to compute 
        if (mAppliedComputeState || mAppliedRayTracingState)
        {
            GraphicsDeviceBase::BindExternalBuffer(buffer, shaderRegister, registerSpace, registerType);
        }
        else if (mAppliedGraphicsState)
        {
            const HAL::RootSignature* signature = mAppliedGraphicsState->GetRootSignature();

            auto index = signature->GetParameterIndex({ shaderRegister, registerSpace, registerType });

            assert_format(index, "Root signature parameter doesn't exist. It either wasn't created or register/space/type aren't correctly specified.");

            switch (registerType)
            {
            case HAL::ShaderRegister::ConstantBuffer:
                mCommandList->SetGraphicsRootConstantBuffer(*buffer.HALBuffer(), index->IndexInSignature);
                break;
            case HAL::ShaderRegister::ShaderResource:
                mCommandList->SetGraphicsRootDescriptorTable(buffer.GetSRDescriptor()->GPUAddress(), index->IndexInSignature);
                break;
            case HAL::ShaderRegister::UnorderedAccess:
                mCommandList->SetGraphicsRootDescriptorTable(buffer.GetUADescriptor()->GPUAddress(), index->IndexInSignature);
                break;
            case HAL::ShaderRegister::Sampler:
                assert_format(false, "Incompatible register type");
            }
        }
        else {
            assert_format(false, "No pipeline state applied");
        }
    }

    void GraphicsDevice::SetBackBuffer(Memory::Texture* backBuffer)
    {
        mBackBuffer = backBuffer;
    }

}
