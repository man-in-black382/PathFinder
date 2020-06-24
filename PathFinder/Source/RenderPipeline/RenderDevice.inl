namespace PathFinder
{

    template <class T>
    void RenderDevice::SetRootConstants(const RenderPassGraph::Node* passNode, const T& constants, uint16_t shaderRegister, uint16_t registerSpace)
    {
        PassHelpers& helpers = mPassHelpers[passNode->GlobalExecutionIndex()];

        assert_format(helpers.LastSetPipelineState, "No pipeline state applied before setting root constants in ", passNode->PassMetadata().Name.ToString(), " render pass");

        // Ray Tracing bindings go to compute 
        if (helpers.LastSetPipelineState->ComputePSO || helpers.LastSetPipelineState->RayTracingPSO)
        {
            const HAL::RootSignature* signature = helpers.LastSetPipelineState->ComputePSO ?
                helpers.LastSetPipelineState->ComputePSO->GetRootSignature() :
                helpers.LastSetPipelineState->RayTracingPSO->GetGlobalRootSignature();

            HAL::ComputeCommandListBase* cmdList = GetComputeCommandListBase(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList);

            auto index = signature->GetParameterIndex({ shaderRegister, registerSpace, HAL::ShaderRegister::ConstantBuffer });
            assert_format(index, "Root signature parameter doesn't exist");
            cmdList->SetComputeRootConstants(constants, index->IndexInSignature);
        }
        else if (helpers.LastSetPipelineState->GraphicPSO)
        {
            const HAL::RootSignature* signature = helpers.LastSetPipelineState->GraphicPSO->GetRootSignature();
            auto cmdList = std::get<GraphicsCommandListPtr>(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList).get();

            auto index = signature->GetParameterIndex({ shaderRegister, registerSpace, HAL::ShaderRegister::ConstantBuffer });
            assert_format(index, "Root signature parameter doesn't exist");
            cmdList->SetGraphicsRootConstants(constants, index->IndexInSignature);
        }
    }

    template <size_t RTCount>
    void RenderDevice::SetRenderTargets(const RenderPassGraph::Node* passNode, const std::array<Foundation::Name, RTCount>& rtNames, std::optional<Foundation::Name> dsName)
    {
        assert_format(RenderPassExecutionQueue{ passNode->ExecutionQueueIndex } != RenderPassExecutionQueue::AsyncCompute,
            "Render Target Set command is unsupported on asynchronous compute queue");

        auto cmdList = std::get<GraphicsCommandListPtr>(mPassCommandLists[passNode->GlobalExecutionIndex()].WorkCommandList).get();
        const HAL::DSDescriptor* dsDescriptor = dsName ? mResourceStorage->GetDepthStencilDescriptor(*dsName, passNode->PassMetadata().Name) : nullptr;

        std::array<const HAL::RTDescriptor*, RTCount> descriptors;

        for (auto i = 0; i < RTCount; ++i)
        {
            descriptors[i] = mResourceStorage->GetRenderTargetDescriptor(rtNames[i], passNode->PassMetadata().Name);
        }

        cmdList->SetRenderTargets(descriptors, dsDescriptor);
    }

}

