namespace PathFinder
{

    template <class T>
    void GraphicsDevice::BindExternalBuffer(const HAL::BufferResource<T>& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        if (mAppliedComputeState)
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

        std::optional<HAL::RootSignature::ParameterIndex> index = std::nullopt;

        if (mAppliedGraphicState)
        {
            index = mAppliedGraphicState->GetRootSignature()->GetParameterIndex({ shaderRegister, registerSpace, registerType });
        }
        else if (mAppliedRayTracingState)
        {
            index = mAppliedRayTracingState->GetGlobalRootSignature()->GetParameterIndex({ shaderRegister, registerSpace, registerType });
        }
        else {
            assert_format("No PSO/Root Signature applied");
            return;
        }

        assert_format(index, "Root signature parameter doesn't exist");
        assert_format(!index->IsIndirect, "Descriptor tables for buffers are not supported. Bind buffers directly instead.");

        switch (registerType)
        {
        case HAL::ShaderRegister::ShaderResource: CommandList().SetGraphicsRootShaderResource(buffer, index->IndexInSignature); break;
        case HAL::ShaderRegister::ConstantBuffer: CommandList().SetGraphicsRootConstantBuffer(buffer, index->IndexInSignature); break;
        case HAL::ShaderRegister::UnorderedAccess: CommandList().SetGraphicsRootUnorderedAccessResource(buffer, index->IndexInSignature); break;
        case HAL::ShaderRegister::Sampler:
            assert_format(false, "Incompatible register type");
        }
    }

}

