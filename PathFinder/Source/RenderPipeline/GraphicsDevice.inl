namespace PathFinder
{

    template <class T>
    void GraphicsDevice::SetRootConstants(const T& constants, uint16_t shaderRegister, uint16_t registerSpace)
    {
        if (mAppliedComputeState || mAppliedRayTracingState)
        {
            GraphicsDeviceBase::SetRootConstants(constants, shaderRegister, registerSpace);
        }
        else if (mAppliedGraphicsState)
        {
            const HAL::RootSignature* signature = mAppliedGraphicsState->GetRootSignature();

            auto index = signature->GetParameterIndex({ shaderRegister, registerSpace,  HAL::ShaderRegister::ConstantBuffer });
            assert_format(index, "Root signature parameter doesn't exist");
            mCommandList->SetGraphicsRootConstants(constants, index->IndexInSignature);
        }
        else {
            assert_format(false, "No pipeline state applied");
        }
    }

    template <size_t RTCount>
    void GraphicsDevice::SetRenderTargets(const std::array<ResourceKey, RTCount>& rtKeys, std::optional<ResourceKey> dsKey)
    {
        const HAL::DSDescriptor* dsDescriptor = dsKey ?
            mResourceStorage->GetDepthStencilDescriptor(dsKey->ResourceName(), dsKey->IndexInArray()) : nullptr;

        std::array<const HAL::RTDescriptor*, RTCount> descriptors;
        
        for (auto i = 0; i < RTCount; ++i)
        {
            descriptors[i] = mResourceStorage->GetRenderTargetDescriptor(rtKeys[i].ResourceName(), rtKeys[i].IndexInArray());
        }

        mCommandList->SetRenderTargets(descriptors, dsDescriptor);
    }

}

