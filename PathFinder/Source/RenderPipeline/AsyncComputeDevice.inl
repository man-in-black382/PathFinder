namespace PathFinder
{

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::AsyncComputeDevice(
        const HAL::Device& device,
        const HAL::CBSRUADescriptorHeap* universalGPUDescriptorHeap,
        PipelineResourceStorage* resourceStorage, 
        PipelineStateManager* pipelineStateManager,
        const RenderSurfaceDescription& defaultRenderSurface, 
        uint8_t simultaneousFramesInFlight)
        :
        mCommandQueue{ device },
        mUniversalGPUDescriptorHeap{ universalGPUDescriptorHeap },
        mRingCommandList{ device, simultaneousFramesInFlight },
        mResourceStorage{ resourceStorage },
        mPipelineStateManager{ pipelineStateManager },
        mDefaultRenderSurface{ defaultRenderSurface }
    {
        mCommandQueue.SetDebugName("Async_Compute_Device_Cmd_Queue");
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::BindBuffer(
        Foundation::Name resourceName, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        const BufferPipelineResource* resource = mResourceStorage->GetPipelineBufferResource(resourceName);
        assert_format(resource, "Buffer ' ", resourceName.ToString(), "' doesn't exist");

        BindExternalBuffer(*resource->Resource, shaderRegister, registerSpace, registerType);
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    template <class T>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::BindExternalBuffer(
        const HAL::BufferResource<T>& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        if (mAppliedComputeState || mAppliedRayTracingState)
        {
            auto index = mAppliedComputeState->GetRootSignature()->GetParameterIndex({ shaderRegister, registerSpace, registerType });

            assert_format(index, "Root signature parameter doesn't exist");

            // Will be changed if I'll see render passes that require a lot of buffers. 
            assert_format(!index->IsIndirect, "Descriptor tables for buffers are not supported. Bind buffers directly instead.");

            switch (registerType)
            {
            case HAL::ShaderRegister::ShaderResource: CommandList().SetComputeRootShaderResource(buffer, index->IndexInSignature); break;
            case HAL::ShaderRegister::ConstantBuffer: CommandList().SetComputeRootConstantBuffer(buffer, index->IndexInSignature); break;
            case HAL::ShaderRegister::UnorderedAccess: CommandList().SetComputeRootUnorderedAccessResource(buffer, index->IndexInSignature); break;
            case HAL::ShaderRegister::Sampler:
                assert_format(false, "Incompatible register type");
            }

            return;
        }
        else {
            assert_format(false, "No pipeline state applied");
        }
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::ApplyCommonComputeResourceBindings()
    {
        CommandList().SetDescriptorHeap(*mUniversalGPUDescriptorHeap);

        // Look at PipelineStateManager for base root signature parameter ordering
        CommandList().SetComputeRootConstantBuffer(mResourceStorage->GlobalRootConstantsBuffer(), 0);
        CommandList().SetComputeRootConstantBuffer(mResourceStorage->PerFrameRootConstantsBuffer(), 1);

        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture2D, 0))
        {
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 3);
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 4);
        }

        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture3D, 0))
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 5);

        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture2DArray, 0))
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 6);

        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture2D, 0))
        {
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 7);
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 8);
        }

        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture3D, 0))
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 9);

        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture2DArray, 0))
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 10);
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::BindCurrentPassConstantBufferCompute()
    {
        if (auto buffer = mResourceStorage->RootConstantBufferForCurrentPass())
        {
            CommandList().SetComputeRootConstantBuffer(*buffer, 2);
        }
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::EndFrame(uint64_t completedFrameFenceValue)
    {
        mRingCommandList.ReleaseAndResetForCompletedFrames(completedFrameFenceValue);
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::BeginFrame(uint64_t frameFenceValue)
    {
        mRingCommandList.PrepareCommandListForNewFrame(frameFenceValue);
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::SignalFence(HAL::Fence& fence)
    {
        mCommandQueue.SignalFence(fence);
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::ResetCommandList()
    {
        mRingCommandList.CurrentCommandList().Reset(mRingCommandList.CurrentCommandAllocator());
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::ExecuteCommands()
    {
        CommandList().Close();
        mCommandQueue.ExecuteCommandList(CommandList());
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::WaitFence(HAL::Fence& fence)
    {
        mCommandQueue.WaitFence(fence);
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {
        CommandList().Dispatch(groupCountX, groupCountY, groupCountZ);
        CommandList().InsertBarriers(mResourceStorage->UnorderedAccessBarriersForCurrentPass());
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::ApplyPipelineState(Foundation::Name psoName)
    {
        const PipelineStateManager::PipelineStateVariant* state = mPipelineStateManager->GetPipelineState(psoName);
        assert_format(state, "Pipeline state doesn't exist");
        assert_format(!std::holds_alternative<HAL::GraphicsPipelineState>(*state), "Trying to apply graphics pipeline state to compute device");

        if (std::holds_alternative<HAL::ComputePipelineState>(*state))
        {
            ApplyStateIfNeeded(&std::get<HAL::ComputePipelineState>(*state));
        }
        else if (std::holds_alternative<HAL::RayTracingPipelineState>(*state))
        {
            ApplyStateIfNeeded(&std::get<HAL::RayTracingPipelineState>(*state));
        } 
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::ApplyStateIfNeeded(const HAL::ComputePipelineState* state)
    {
        bool computeStateApplied = mAppliedComputeState != nullptr;

        // State is already applied
        //
        if (computeStateApplied && mAppliedComputeState == state) return;

        CommandList().SetPipelineState(*state);

        // Skip setting common bindings when workload type and root signature are not going to change
        //
        if (computeStateApplied && state->GetRootSignature() == mAppliedComputeRootSignature)
        {
            BindCurrentPassConstantBufferCompute();
            return;
        }

        CommandList().SetComputeRootSignature(*state->GetRootSignature());
        ApplyCommonComputeResourceBindings();
        BindCurrentPassConstantBufferCompute();

        mAppliedComputeRootSignature = state->GetRootSignature();
        mAppliedComputeState = state;
        mAppliedRayTracingState = nullptr;
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::ApplyStateIfNeeded(const HAL::RayTracingPipelineState* state)
    {
        bool rayTracingStateApplied = mAppliedRayTracingState != nullptr;

        // State is already applied
        //
        if (rayTracingStateApplied && mAppliedRayTracingState == state) return;

        // Set RT state
        //CommandList().SetPipelineState(*pso);

        // Skip setting common bindings when workload type and root signature are not going to change
        //
        if (rayTracingStateApplied && state->GetGlobalRootSignature() == mAppliedComputeRootSignature)
        {
            BindCurrentPassConstantBufferCompute();
            return;
        }

        CommandList().SetComputeRootSignature(*state->GetGlobalRootSignature());
        ApplyCommonComputeResourceBindings();
        BindCurrentPassConstantBufferCompute();

        mAppliedComputeRootSignature = state->GetGlobalRootSignature();
        mAppliedRayTracingState = state;
        mAppliedComputeState = nullptr;
    }

}

