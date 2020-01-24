namespace PathFinder
{

    template <class T>
    void GraphicsDevice::SetRootConstants(const T& constants, uint16_t shaderRegister, uint16_t registerSpace)
    {
        if (mAppliedComputeState)
        {
            GraphicsDeviceBase::SetRootConstants(constants, shaderRegister, registerSpace);
        }
        else if (mAppliedGraphicsState || mAppliedRayTracingState)
        {
            const HAL::RootSignature* signature = mAppliedGraphicsState ?
                mAppliedGraphicsState->GetRootSignature() : mAppliedRayTracingState->GetGlobalRootSignature();

            auto index = signature->GetParameterIndex({ shaderRegister, registerSpace,  HAL::ShaderRegister::ConstantBuffer });
            assert_format(index, "Root signature parameter doesn't exist");
            CommandList().SetGraphicsRootConstants(constants, index->IndexInSignature);
        }
        else {
            assert_format(false, "No pipeline state applied");
        }
    }

}

