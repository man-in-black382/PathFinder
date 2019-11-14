namespace PathFinder
{

    template <class T>
    void GraphicsDevice::BindExternalBuffer(const HAL::BufferResource<T>& buffer, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
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
            case HAL::ShaderRegister::ShaderResource: CommandList().SetGraphicsRootShaderResource(buffer, index->IndexInSignature); break;
            case HAL::ShaderRegister::ConstantBuffer: CommandList().SetGraphicsRootConstantBuffer(buffer, index->IndexInSignature); break;
            case HAL::ShaderRegister::UnorderedAccess: CommandList().SetGraphicsRootUnorderedAccessResource(buffer, index->IndexInSignature); break;
            case HAL::ShaderRegister::Sampler:
                assert_format(false, "Incompatible register type");
            }
        }
        else {
            assert_format(false, "No pipeline state applied");
        }
    }

}

