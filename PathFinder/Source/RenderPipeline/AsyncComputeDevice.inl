#include "../Foundation/StringUtils.hpp"

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
        mCommandQueue.SetDebugName("Async Compute Device Command Queue");
        mRingCommandList.SetDebugName("Async Compute Device");
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
    template <class T>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::SetRootConstants(
        const T& constants, uint16_t shaderRegister, uint16_t registerSpace)
    {
        auto index = mAppliedComputeState->GetRootSignature()->GetParameterIndex({shaderRegister, registerSpace,  HAL::ShaderRegister::ConstantBuffer });
        assert_format(index, "Root signature parameter doesn't exist");
        CommandList().SetComputeRootConstants(constants, index->IndexInSignature);
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
        {
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 5);
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 6);
        }

        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::Texture2DArray, 0))
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 7);

        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture2D, 0))
        {
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 8);
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 9);
        }

        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture3D, 0))
        {
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 10);
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 11);
        }

        if (auto baseDescriptor = mUniversalGPUDescriptorHeap->GetDescriptor(HAL::CBSRUADescriptorHeap::Range::UATexture2DArray, 0))
            CommandList().SetComputeRootDescriptorTable(*baseDescriptor, 12);
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::BindCurrentPassBuffersCompute()
    {
        if (auto buffer = mResourceStorage->RootConstantBufferForCurrentPass())
        {
            CommandList().SetComputeRootConstantBuffer(*buffer, 2);
        }

        CommandList().SetComputeRootUnorderedAccessResource(*mResourceStorage->DebugBufferForCurrentPass(), 13);
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::EndFrame(uint64_t completedFrameNumber)
    {
        mRingCommandList.ReleaseAndResetForCompletedFrames(completedFrameNumber);
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::BeginFrame(uint64_t newFrameNumber)
    {
        mRingCommandList.PrepareCommandListForNewFrame(newFrameNumber);
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::ResetCommandList()
    {
        mRingCommandList.CurrentCommandList().Reset(mRingCommandList.CurrentCommandAllocator());
    }

    template <class CommandListT, class CommandAllocatorT, class CommandQueueT>
    void AsyncComputeDevice<CommandListT, CommandAllocatorT, CommandQueueT>::ExecuteCommands(const HAL::Fence* fenceToWaitFor, const HAL::Fence* fenceToSignal)
    {
        if (fenceToWaitFor)
        {
            mCommandQueue.WaitFence(*fenceToWaitFor);
        }

        CommandList().Close();
        mCommandQueue.ExecuteCommandList(CommandList());

        if (fenceToSignal)
        {
            mCommandQueue.SignalFence(*fenceToSignal);
        }        
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
            BindCurrentPassBuffersCompute();
            return;
        }

        CommandList().SetComputeRootSignature(*state->GetRootSignature());
        ApplyCommonComputeResourceBindings();
        BindCurrentPassBuffersCompute();

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
            BindCurrentPassBuffersCompute();
            return;
        }

        CommandList().SetComputeRootSignature(*state->GetGlobalRootSignature());
        ApplyCommonComputeResourceBindings();
        BindCurrentPassBuffersCompute();

        mAppliedComputeRootSignature = state->GetGlobalRootSignature();
        mAppliedRayTracingState = state;
        mAppliedComputeState = nullptr;
    }

}

