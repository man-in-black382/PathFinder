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
    void GraphicsDevice::SetRenderTargets(const std::array<Foundation::Name, RTCount>& rtResourceNames, std::optional<Foundation::Name> depthStencilResourceName)
    {
        const HAL::DSDescriptor* dsDescriptor = depthStencilResourceName ? mResourceStorage->GetDepthStencilDescriptor(*depthStencilResourceName) : nullptr;
        std::array<const HAL::RTDescriptor*, RTCount> descriptors;
        
        for (auto i = 0; i < RTCount; ++i)
        {
            descriptors[i] = mResourceStorage->GetRenderTargetDescriptor(rtResourceNames[i]);
        }

        mCommandList->SetRenderTargets(descriptors, dsDescriptor);
    }

}

