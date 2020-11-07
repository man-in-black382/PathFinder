#include "CommandRecorder.hpp"

namespace PathFinder
{

    CommandRecorder::CommandRecorder(
        RenderDevice* renderDevice, 
        PipelineResourceStorage* resourceStorage, 
        PipelineStateManager* pipelineStateManager,
        Memory::PoolDescriptorAllocator* descriptorAllocator,
        const RenderPassGraph* passGraph, 
        uint64_t graphNodeIndex)
        : 
        mRenderDevice{ renderDevice }, 
        mResourceStorage{ resourceStorage }, 
        mPipelineStateManager{ pipelineStateManager },
        mDescriptorAllocator{ descriptorAllocator },
        mPassGraph{ passGraph },
        mGraphNodeIndex{ graphNodeIndex } {}

     Geometry::Dimensions CommandRecorder::DispatchGroupCount(const Geometry::Dimensions& viewportDimensions, const Geometry::Dimensions& groupSize)
     {
         uint64_t x = std::max(ceilf((float)viewportDimensions.Width / groupSize.Width), 1.f);
         uint64_t y = std::max(ceilf((float)viewportDimensions.Height / groupSize.Height), 1.f);
         uint64_t z = std::max(ceilf((float)viewportDimensions.Depth / groupSize.Depth), 1.f);

         return { x,y,z };
     }

    void CommandRecorder::SetRenderTarget(Foundation::Name rtName, std::optional<Foundation::Name> dsName)
    {
        assert_format(RenderPassExecutionQueue{ GetPassNode().ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Render Target Set command is unsupported on asynchronous compute queue");

        const HAL::DSDescriptor* dsDescriptor = dsName ? mResourceStorage->GetDepthStencilDescriptor(*dsName, GetPassNode().PassMetadata().Name) : nullptr;
        GetGraphicsCommandList()->SetRenderTarget(*mResourceStorage->GetRenderTargetDescriptor(rtName, GetPassNode().PassMetadata().Name), dsDescriptor);
    }

    void CommandRecorder::SetBackBufferAsRenderTarget(std::optional<Foundation::Name> dsName)
    {
        assert_format(RenderPassExecutionQueue{ GetPassNode().ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Render Target Set command is unsupported on asynchronous compute queue");

        assert_format(GetPassNode().HasDependency(RenderPassGraph::Node::BackBufferName, 0), "Render pass has not scheduled writing to back buffer");

        const HAL::DSDescriptor* dsDescriptor = dsName ? mResourceStorage->GetDepthStencilDescriptor(*dsName, GetPassNode().PassMetadata().Name) : nullptr;
        GetGraphicsCommandList()->SetRenderTarget(*mRenderDevice->BackBuffer()->GetRTDescriptor(), dsDescriptor);
    }

    void CommandRecorder::ClearRenderTarget(Foundation::Name rtName)
    {
        assert_format(RenderPassExecutionQueue{ GetPassNode().ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Render Target Clear command is unsupported on asynchronous compute queue");

        const Memory::Texture* renderTarget = mResourceStorage->GetPerResourceData(rtName)->Texture.get();
        assert_format(renderTarget, "Render target doesn't exist");

        auto clearValue = std::get_if<HAL::ColorClearValue>(&renderTarget->Properties().OptimizedClearValue);
        assert_format(clearValue, "Texture does not contain optimized color clear value");

        GetGraphicsCommandList()->ClearRenderTarget(*mResourceStorage->GetRenderTargetDescriptor(rtName, GetPassNode().PassMetadata().Name), *clearValue);
    }

    void CommandRecorder::ClearDepth(Foundation::Name dsName)
    {
        assert_format(RenderPassExecutionQueue{ GetPassNode().ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Depth Clear command is unsupported on asynchronous compute queue");

        const Memory::Texture* depthAttachment = mResourceStorage->GetPerResourceData(dsName)->Texture.get();
        assert_format(depthAttachment, "Depth/stencil attachment doesn't exist");

        auto clearValue = std::get_if<HAL::DepthStencilClearValue>(&depthAttachment->Properties().OptimizedClearValue);
        assert_format(clearValue, "Texture does not contain optimized depth/stencil clear value");

        GetGraphicsCommandList()->CleadDepthStencil(*mResourceStorage->GetDepthStencilDescriptor(dsName, GetPassNode().PassMetadata().Name), clearValue->Depth);
    }

    void CommandRecorder::SetViewport(const HAL::Viewport& viewport)
    {
        assert_format(RenderPassExecutionQueue{ GetPassNode().ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Viewport Set command is unsupported on asynchronous compute queue");

        RenderDevice::PassHelpers& passHelpers = GetPassHelpers();
        passHelpers.LastAppliedViewport = viewport;

        GetGraphicsCommandList()->SetViewport(viewport);
    }

    void CommandRecorder::SetScissor(const Geometry::Rect2D& scissor)
    {
        assert_format(RenderPassExecutionQueue{ GetPassNode().ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Viewport Rect Set command is unsupported on asynchronous compute queue");

        RenderDevice::PassHelpers& passHelpers = GetPassHelpers();
        passHelpers.LastAppliedScissor = scissor;

        GetGraphicsCommandList()->SetScissor(scissor);
    }

    void CommandRecorder::Draw(uint32_t vertexCount, uint32_t instanceCount)
    {
        assert_format(RenderPassExecutionQueue{ GetPassNode().ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Draw command is unsupported on asynchronous compute queue");

        HAL::GraphicsCommandList* cmdList = GetGraphicsCommandList();
        RenderDevice::PassHelpers& passHelpers = GetPassHelpers();

        // Apply default viewport if none were provided by the render pass yet
        if (!passHelpers.LastAppliedViewport)
        {
            passHelpers.LastAppliedViewport = HAL::Viewport(
                mRenderDevice->DefaultRenderSurfaceDesc().Dimensions().Width,
                mRenderDevice->DefaultRenderSurfaceDesc().Dimensions().Height 
            );

            cmdList->SetViewport(*passHelpers.LastAppliedViewport);
        }

        // Apply default scissor if none were provided by the render pass yet
        if (!passHelpers.LastAppliedScissor)
        {
            passHelpers.LastAppliedScissor = Geometry::Rect2D{
                {0, 0},
                Geometry::Size2D(
                    mRenderDevice->DefaultRenderSurfaceDesc().Dimensions().Width,
                    mRenderDevice->DefaultRenderSurfaceDesc().Dimensions().Height)
            };

            cmdList->SetScissor(*passHelpers.LastAppliedScissor);
        }

        // Inset UAV barriers between draws
        if (passHelpers.ExecutedRenderCommandsCount > 0)
        {
            cmdList->InsertBarriers(passHelpers.UAVBarriers);
        }

        BindGraphicsPassRootConstantBuffer(cmdList);
        cmdList->Draw(vertexCount, 0);

        passHelpers.ResourceStoragePassData->IsAllowedToAdvanceConstantBufferOffset = true;
        passHelpers.ExecutedRenderCommandsCount++;
    }

    void CommandRecorder::Draw(const DrawablePrimitive& primitive)
    {
        Draw(primitive.VertexCount());
    }

    void CommandRecorder::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase();
        RenderDevice::PassHelpers& passHelpers = GetPassHelpers();

        if (passHelpers.ExecutedRenderCommandsCount > 0)
        {
            cmdList->InsertBarriers(passHelpers.UAVBarriers);
        }

        BindComputePassRootConstantBuffer(cmdList);
        cmdList->Dispatch(groupCountX, groupCountY, groupCountZ);

        passHelpers.ResourceStoragePassData->IsAllowedToAdvanceConstantBufferOffset = true;
        passHelpers.ExecutedRenderCommandsCount++;
    }

    void CommandRecorder::Dispatch(const Geometry::Dimensions& viewportDimensions, const Geometry::Dimensions& groupSize)
    {
        auto dims = DispatchGroupCount(viewportDimensions, groupSize);
        Dispatch(dims.Width, dims.Height, dims.Depth);
    }

    void CommandRecorder::DispatchRays(const Geometry::Dimensions& dispatchDimensions)
    {
        RenderDevice::PassHelpers& passHelpers = GetPassHelpers();

        assert_format(passHelpers.LastAppliedRTStateDispatchInfo, "No Ray Tracing state / dispatch info were applied before Ray Dispatch");

        HAL::RayDispatchInfo dispatchInfo = *passHelpers.LastAppliedRTStateDispatchInfo;
        dispatchInfo.SetWidth(dispatchDimensions.Width);
        dispatchInfo.SetHeight(dispatchDimensions.Height);
        dispatchInfo.SetDepth(dispatchDimensions.Depth);

        HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase();

        if (passHelpers.ExecutedRenderCommandsCount > 0)
        {
            cmdList->InsertBarriers(passHelpers.UAVBarriers);
        }

        BindComputePassRootConstantBuffer(cmdList);
        cmdList->DispatchRays(dispatchInfo);
        passHelpers.ResourceStoragePassData->IsAllowedToAdvanceConstantBufferOffset = true;
        passHelpers.ExecutedRenderCommandsCount++;
    }

    void CommandRecorder::BindBuffer(Foundation::Name resourceName, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        const PipelineResourceStorageResource* resourceData = mResourceStorage->GetPerResourceData(resourceName);
        assert_format(resourceData->Buffer, "Buffer ' ", resourceName.ToString(), "' doesn't exist");

        BindExternalBuffer(*resourceData->Buffer, shaderRegister, registerSpace, registerType);
    }

    void CommandRecorder::BindGraphicsCommonResources(const HAL::RootSignature* rootSignature, HAL::GraphicsCommandListBase* cmdList)
    {
        auto commonParametersIndexOffset = rootSignature->ParameterCount() - mPipelineStateManager->CommonRootSignatureParameterCount();

        // Look at PipelineStateManager for base root signature parameter ordering
        HAL::DescriptorAddress SRRangeAddress = mDescriptorAllocator->CBSRUADescriptorHeap().RangeStartGPUAddress(HAL::CBSRUADescriptorHeap::Range::ShaderResource);
        HAL::DescriptorAddress UARangeAddress = mDescriptorAllocator->CBSRUADescriptorHeap().RangeStartGPUAddress(HAL::CBSRUADescriptorHeap::Range::UnorderedAccess);
        HAL::DescriptorAddress samplerRangeAddress = mDescriptorAllocator->SamplerDescriptorHeap().StartGPUAddress();

        const RenderDevice::PassHelpers& passHelpers = GetPassHelpers();

        // Alias different registers to one GPU address
        cmdList->SetGraphicsRootDescriptorTable(SRRangeAddress, 3 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(SRRangeAddress, 4 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(SRRangeAddress, 5 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(SRRangeAddress, 6 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(SRRangeAddress, 7 + commonParametersIndexOffset);

        cmdList->SetGraphicsRootDescriptorTable(UARangeAddress, 8 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(UARangeAddress, 9 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(UARangeAddress, 10 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(UARangeAddress, 11 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(UARangeAddress, 12 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootDescriptorTable(UARangeAddress, 13 + commonParametersIndexOffset);

        cmdList->SetGraphicsRootDescriptorTable(samplerRangeAddress, 14 + commonParametersIndexOffset);

        cmdList->SetGraphicsRootConstantBuffer(*mResourceStorage->GlobalRootConstantsBuffer()->HALBuffer(), 0 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootConstantBuffer(*mResourceStorage->PerFrameRootConstantsBuffer()->HALBuffer(), 1 + commonParametersIndexOffset);
        cmdList->SetGraphicsRootUnorderedAccessResource(*passHelpers.ResourceStoragePassData->PassDebugBuffer->HALBuffer(), 15 + commonParametersIndexOffset);
    }

    void CommandRecorder::BindComputeCommonResources(const HAL::RootSignature* rootSignature, HAL::ComputeCommandListBase* cmdList)
    {
        auto commonParametersIndexOffset = rootSignature->ParameterCount() - mPipelineStateManager->CommonRootSignatureParameterCount();

        // Look at PipelineStateManager for base root signature parameter ordering
        HAL::DescriptorAddress SRRangeAddress = mDescriptorAllocator->CBSRUADescriptorHeap().RangeStartGPUAddress(HAL::CBSRUADescriptorHeap::Range::ShaderResource);
        HAL::DescriptorAddress UARangeAddress = mDescriptorAllocator->CBSRUADescriptorHeap().RangeStartGPUAddress(HAL::CBSRUADescriptorHeap::Range::UnorderedAccess);
        HAL::DescriptorAddress samplerRangeAddress = mDescriptorAllocator->SamplerDescriptorHeap().StartGPUAddress();

        const RenderDevice::PassHelpers& passHelpers = GetPassHelpers();

        // Alias different registers to one GPU address
        cmdList->SetComputeRootDescriptorTable(SRRangeAddress, 3 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(SRRangeAddress, 4 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(SRRangeAddress, 5 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(SRRangeAddress, 6 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(SRRangeAddress, 7 + commonParametersIndexOffset);

        cmdList->SetComputeRootDescriptorTable(UARangeAddress, 8 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(UARangeAddress, 9 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(UARangeAddress, 10 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(UARangeAddress, 11 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(UARangeAddress, 12 + commonParametersIndexOffset);
        cmdList->SetComputeRootDescriptorTable(UARangeAddress, 13 + commonParametersIndexOffset);

        cmdList->SetComputeRootDescriptorTable(samplerRangeAddress, 14 + commonParametersIndexOffset);

        cmdList->SetComputeRootConstantBuffer(*mResourceStorage->GlobalRootConstantsBuffer()->HALBuffer(), 0 + commonParametersIndexOffset);
        cmdList->SetComputeRootConstantBuffer(*mResourceStorage->PerFrameRootConstantsBuffer()->HALBuffer(), 1 + commonParametersIndexOffset);
        cmdList->SetComputeRootUnorderedAccessResource(*passHelpers.ResourceStoragePassData->PassDebugBuffer->HALBuffer(), 15 + commonParametersIndexOffset);
    }

    void CommandRecorder::BindGraphicsPassRootConstantBuffer(HAL::GraphicsCommandListBase* cmdList)
    {
        RenderDevice::PassHelpers& passHelpers = GetPassHelpers();
        CheckSignatureAndStatePresense(passHelpers);

        auto commonParametersIndexOffset = passHelpers.LastSetRootSignature->ParameterCount() - mPipelineStateManager->CommonRootSignatureParameterCount();

        if (!passHelpers.ResourceStoragePassData->PassConstantBuffer)
        {
            return;
        }

        HAL::GPUAddress address =
            passHelpers.ResourceStoragePassData->PassConstantBuffer->HALBuffer()->GPUVirtualAddress() +
            passHelpers.ResourceStoragePassData->PassConstantBufferMemoryOffset;

        // Already bound
        if (passHelpers.LastBoundRootConstantBufferAddress == address)
        {
            return;
        }

        passHelpers.LastBoundRootConstantBufferAddress = address;
        cmdList->SetGraphicsRootConstantBuffer(address, 2 + commonParametersIndexOffset);
    }

    void CommandRecorder::BindComputePassRootConstantBuffer(HAL::ComputeCommandListBase* cmdList)
    {
        RenderDevice::PassHelpers& passHelpers = GetPassHelpers();
        CheckSignatureAndStatePresense(passHelpers);

        auto commonParametersIndexOffset = passHelpers.LastSetRootSignature->ParameterCount() - mPipelineStateManager->CommonRootSignatureParameterCount();

        if (!passHelpers.ResourceStoragePassData->PassConstantBuffer)
        {
            return;
        }

        HAL::GPUAddress address =
            passHelpers.ResourceStoragePassData->PassConstantBuffer->HALBuffer()->GPUVirtualAddress() +
            passHelpers.ResourceStoragePassData->PassConstantBufferMemoryOffset;

        // Already bound
        if (passHelpers.LastBoundRootConstantBufferAddress == address)
        {
            return;
        }

        passHelpers.LastBoundRootConstantBufferAddress = address;
        cmdList->SetComputeRootConstantBuffer(address, 2 + commonParametersIndexOffset);
    }

    void CommandRecorder::ApplyPipelineState(Foundation::Name psoName)
    {
        std::optional<PipelineStateManager::PipelineStateVariant> state = mPipelineStateManager->GetPipelineState(psoName);
        assert_format(state, "Pipeline state doesn't exist");

        if (state->ComputePSO) ApplyState(state->ComputePSO);
        else if (state->GraphicPSO) ApplyState(state->GraphicPSO);
        else if (state->RayTracingPSO) ApplyState(state->RayTracingPSO, state->BaseRayDispatchInfo);

        RenderDevice::PassHelpers& passHelpers = GetPassHelpers();
        passHelpers.LastSetPipelineState = state;
    }

    void CommandRecorder::ApplyState(const HAL::GraphicsPipelineState* state)
    {
        RenderPassExecutionQueue queueType{ GetPassNode().ExecutionQueueIndex };
        assert_format(queueType != RenderPassExecutionQueue::AsyncCompute, "Cannot apply Graphics State on Async Compute queue");
        HAL::GraphicsCommandList* cmdList = GetGraphicsCommandList();

        cmdList->SetPipelineState(*state);
        cmdList->SetPrimitiveTopology(state->GetPrimitiveTopology());
        cmdList->SetGraphicsRootSignature(*state->GetRootSignature());

        RenderDevice::PassHelpers& passHelpers = GetPassHelpers();
        passHelpers.LastSetRootSignature = state->GetRootSignature();

        BindGraphicsCommonResources(state->GetRootSignature(), cmdList);
    }

    void CommandRecorder::ApplyState(const HAL::ComputePipelineState* state)
    {
        HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase();

        cmdList->SetPipelineState(*state);
        cmdList->SetComputeRootSignature(*state->GetRootSignature());

        RenderDevice::PassHelpers& passHelpers = GetPassHelpers();
        passHelpers.LastSetRootSignature = state->GetRootSignature();

        BindComputeCommonResources(state->GetRootSignature(), cmdList);
    }

    void CommandRecorder::ApplyState(const HAL::RayTracingPipelineState* state, const HAL::RayDispatchInfo* dispatchInfo)
    {
        assert_format(GetPassNode().UsesRayTracing, "Render pass ", GetPassNode().PassMetadata().Name.ToString(), " didn't schedule Ray Tracing usage");

        HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase();

        cmdList->SetPipelineState(*state);
        cmdList->SetComputeRootSignature(*state->GetGlobalRootSignature());

        RenderDevice::PassHelpers& passHelpers = GetPassHelpers();
        passHelpers.LastSetRootSignature = state->GetGlobalRootSignature();
        passHelpers.LastAppliedRTStateDispatchInfo = dispatchInfo;

        BindComputeCommonResources(state->GetGlobalRootSignature(), cmdList);
    }

    void CommandRecorder::BindExternalBuffer(const Memory::Buffer& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        RenderDevice::PassHelpers& helpers = GetPassHelpers();

        assert_format(helpers.LastSetPipelineState, "No pipeline state applied before binding a buffer in ", GetPassNode().PassMetadata().Name.ToString(), " render pass");

        // Ray Tracing bindings go to compute 
        if (helpers.LastSetPipelineState->ComputePSO || helpers.LastSetPipelineState->RayTracingPSO)
        {
            const HAL::RootSignature* signature = helpers.LastSetPipelineState->ComputePSO ?
                helpers.LastSetPipelineState->ComputePSO->GetRootSignature() :
                helpers.LastSetPipelineState->RayTracingPSO->GetGlobalRootSignature();

            HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase();

            auto index = signature->GetParameterIndex({ shaderRegister, registerSpace, registerType });

            assert_format(index, "Root signature parameter doesn't exist. It either wasn't created or register/space/type aren't correctly specified.");

            switch (registerType)
            {
            case HAL::ShaderRegister::ConstantBuffer: cmdList->SetComputeRootConstantBuffer(*buffer.HALBuffer(), index->IndexInSignature); break;
            case HAL::ShaderRegister::ShaderResource: cmdList->SetComputeRootDescriptorTable(buffer.GetSRDescriptor()->GPUAddress(), index->IndexInSignature); break;
            case HAL::ShaderRegister::UnorderedAccess: cmdList->SetComputeRootDescriptorTable(buffer.GetUADescriptor()->GPUAddress(), index->IndexInSignature); break;
            case HAL::ShaderRegister::Sampler: assert_format(false, "Incompatible register type");
            }
        }
        else if (helpers.LastSetPipelineState->GraphicPSO)
        {
            const HAL::RootSignature* signature = helpers.LastSetPipelineState->GraphicPSO->GetRootSignature();
            HAL::GraphicsCommandList* cmdList = GetGraphicsCommandList();
            auto index = signature->GetParameterIndex({ shaderRegister, registerSpace, registerType });

            assert_format(index, "Root signature parameter doesn't exist. It either wasn't created or register/space/type aren't correctly specified.");

            switch (registerType)
            {
            case HAL::ShaderRegister::ConstantBuffer: cmdList->SetGraphicsRootConstantBuffer(*buffer.HALBuffer(), index->IndexInSignature); break;
            case HAL::ShaderRegister::ShaderResource: cmdList->SetGraphicsRootDescriptorTable(buffer.GetSRDescriptor()->GPUAddress(), index->IndexInSignature); break;
            case HAL::ShaderRegister::UnorderedAccess: cmdList->SetGraphicsRootDescriptorTable(buffer.GetUADescriptor()->GPUAddress(), index->IndexInSignature); break;
            case HAL::ShaderRegister::Sampler: assert_format(false, "Incompatible register type");
            }
        }
    }

    void CommandRecorder::CheckSignatureAndStatePresense(const RenderDevice::PassHelpers& passHelpers) const
    {
        assert_format(passHelpers.LastSetPipelineState != std::nullopt, "No pipeline state was set in this render pass");
        assert_format(passHelpers.LastSetRootSignature != nullptr, "No root signature was set in this render pass");
    }

    const RenderPassGraph::Node& CommandRecorder::GetPassNode() const
    {
        return mPassGraph->Nodes()[mGraphNodeIndex];
    }

    HAL::GraphicsCommandList* CommandRecorder::GetGraphicsCommandList() const
    {
        return std::get<RenderDevice::GraphicsCommandListPtr>(mRenderDevice->CommandListsForNode(GetPassNode()).WorkCommandList).get();
    }

    HAL::ComputeCommandListBase* CommandRecorder::GetComputeCommandListBase() const
    {
        return mRenderDevice->GetComputeCommandListBase(mRenderDevice->CommandListsForNode(GetPassNode()).WorkCommandList);
    }

    RenderDevice::PassHelpers& CommandRecorder::GetPassHelpers() const
    {
        return mRenderDevice->PassHelpersForNode(GetPassNode());
    }

}
