#include <aftermath/AftermathHelpers.hpp>

namespace PathFinder
{

    template <class Lambda>
    void RenderDevice::RecordWorkerCommandList(const RenderPassGraph::Node& passNode, const Lambda& action)
    {
        HAL::ComputeCommandListBase* worker = GetComputeCommandListBase(mPassCommandLists[passNode.GlobalExecutionIndex()].WorkCommandList);
        worker->Reset();

        const std::string& passName = passNode.PassMetadata().Name.ToString();
        mEventTracker.StartGPUEvent(passName, *worker);

        if (worker->AftermathHandle())
        {
            AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_SetEventMarker(*worker->AftermathHandle(), passName.c_str(), passName.size() + 1));
        }

        worker->SetDescriptorHeap(*mUniversalGPUDescriptorHeap);
        action();
        mEventTracker.EndGPUEvent(*worker);
        worker->Close();
    }

    template <class T>
    void RenderDevice::SetRootConstants(const RenderPassGraph::Node& passNode, const T& constants, uint16_t shaderRegister, uint16_t registerSpace)
    {
        PassHelpers& helpers = mPassHelpers[passNode.GlobalExecutionIndex()];

        assert_format(helpers.LastSetPipelineState, "No pipeline state applied before setting root constants in ", passNode.PassMetadata().Name.ToString(), " render pass");

        // Ray Tracing bindings go to compute 
        if (helpers.LastSetPipelineState->ComputePSO || helpers.LastSetPipelineState->RayTracingPSO)
        {
            const HAL::RootSignature* signature = helpers.LastSetPipelineState->ComputePSO ?
                helpers.LastSetPipelineState->ComputePSO->GetRootSignature() :
                helpers.LastSetPipelineState->RayTracingPSO->GetGlobalRootSignature();

            HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase(mPassCommandLists[passNode.GlobalExecutionIndex()].WorkCommandList);

            auto index = signature->GetParameterIndex({ shaderRegister, registerSpace, HAL::ShaderRegister::ConstantBuffer });
            assert_format(index, "Root signature parameter doesn't exist");
            cmdList->SetComputeRootConstants(constants, index->IndexInSignature);
        }
        else if (helpers.LastSetPipelineState->GraphicPSO)
        {
            const HAL::RootSignature* signature = helpers.LastSetPipelineState->GraphicPSO->GetRootSignature();
            auto cmdList = std::get<GraphicsCommandListPtr>(mPassCommandLists[passNode.GlobalExecutionIndex()].WorkCommandList).get();

            auto index = signature->GetParameterIndex({ shaderRegister, registerSpace, HAL::ShaderRegister::ConstantBuffer });
            assert_format(index, "Root signature parameter doesn't exist");
            cmdList->SetGraphicsRootConstants(constants, index->IndexInSignature);
        }
    }

    template <size_t RTCount>
    void RenderDevice::SetRenderTargets(const RenderPassGraph::Node& passNode, const std::array<Foundation::Name, RTCount>& rtNames, std::optional<Foundation::Name> dsName)
    {
        assert_format(RenderPassExecutionQueue{ passNode.ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Render Target Set command is unsupported on asynchronous compute queue");

        auto cmdList = std::get<GraphicsCommandListPtr>(mPassCommandLists[passNode.GlobalExecutionIndex()].WorkCommandList).get();
        const HAL::DSDescriptor* dsDescriptor = dsName ? mResourceStorage->GetDepthStencilDescriptor(*dsName, passNode.PassMetadata().Name) : nullptr;

        std::array<const HAL::RTDescriptor*, RTCount> descriptors;

        for (auto i = 0; i < RTCount; ++i)
        {
            descriptors[i] = mResourceStorage->GetRenderTargetDescriptor(rtNames[i], passNode.PassMetadata().Name);
        }

        cmdList->SetRenderTargets(descriptors, dsDescriptor);
    }

    template <class CommandQueueT, class CommandListT>
    void RenderDevice::ExecuteCommandListBatch(CommandListBatch& batch, HAL::CommandQueue& queue)
    {
        std::vector<CommandListT*> commandLists;
        CommandQueueT* concreteQueue = static_cast<CommandQueueT*>(&queue);

        for (auto cmdListIdx = 0; cmdListIdx < batch.CommandLists.size(); ++cmdListIdx)
        {
            HALCommandListPtrVariant& cmdListVariant = batch.CommandLists[cmdListIdx];
            commandLists.push_back(std::get<CommandListT*>(cmdListVariant));
        }

        concreteQueue->ExecuteCommandLists(commandLists.data(), commandLists.size());
    }

}

