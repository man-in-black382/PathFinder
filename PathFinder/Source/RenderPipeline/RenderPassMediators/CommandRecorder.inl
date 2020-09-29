namespace PathFinder
{

    template <class T>
    void CommandRecorder::SetRootConstants(const T& constants, uint16_t shaderRegister, uint16_t registerSpace)
    {
        RenderDevice::PassHelpers& helpers = GetPassHelpers();

        assert_format(helpers.LastSetPipelineState, "No pipeline state applied before setting root constants in ", GetPassNode().PassMetadata().Name.ToString(), " render pass");

        // Ray Tracing bindings go to compute 
        if (helpers.LastSetPipelineState->ComputePSO || helpers.LastSetPipelineState->RayTracingPSO)
        {
            const HAL::RootSignature* signature = helpers.LastSetPipelineState->ComputePSO ?
                helpers.LastSetPipelineState->ComputePSO->GetRootSignature() :
                helpers.LastSetPipelineState->RayTracingPSO->GetGlobalRootSignature();

            HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase();

            auto index = signature->GetParameterIndex({ shaderRegister, registerSpace, HAL::ShaderRegister::ConstantBuffer });
            assert_format(index, "Root signature parameter doesn't exist");
            cmdList->SetComputeRootConstants(constants, index->IndexInSignature);
        }
        else if (helpers.LastSetPipelineState->GraphicPSO)
        {
            const HAL::RootSignature* signature = helpers.LastSetPipelineState->GraphicPSO->GetRootSignature();
            HAL::GraphicsCommandList* cmdList = GetGraphicsCommandList();

            auto index = signature->GetParameterIndex({ shaderRegister, registerSpace, HAL::ShaderRegister::ConstantBuffer });
            assert_format(index, "Root signature parameter doesn't exist");
            cmdList->SetGraphicsRootConstants(constants, index->IndexInSignature);
        }
    }

    template <size_t RTCount>
    void CommandRecorder::SetRenderTargets(const std::array<Foundation::Name, RTCount>& rtNames, std::optional<Foundation::Name> dsName)
    {
        assert_format(RenderPassExecutionQueue{ GetPassNode().ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Render Target Set command is unsupported on asynchronous compute queue");

        HAL::GraphicsCommandList* cmdList = GetGraphicsCommandList();
        const HAL::DSDescriptor* dsDescriptor = dsName ? mResourceStorage->GetDepthStencilDescriptor(*dsName, GetPassNode().PassMetadata().Name) : nullptr;

        std::array<const HAL::RTDescriptor*, RTCount> descriptors;

        for (auto i = 0; i < RTCount; ++i)
        {
            descriptors[i] = mResourceStorage->GetRenderTargetDescriptor(rtNames[i], GetPassNode().PassMetadata().Name);
        }

        cmdList->SetRenderTargets(descriptors, dsDescriptor);
    }

}

