namespace PathFinder
{

    template <class T>
    void GPUCommandRecorder::SetRootConstants(const T& constants, uint16_t shaderRegister, uint16_t registerSpace)
    {
        mGraphicsDevice->SetRootConstants(constants, shaderRegister, registerSpace);
    }

    template <size_t RTCount>
    void GPUCommandRecorder::SetRenderTargets(const std::array<Foundation::Name, RTCount>& rtResourceNames, std::optional<Foundation::Name> depthStencilResourceName)
    {
        mGraphicsDevice->SetRenderTargets(rtResourceNames, depthStencilResourceName);
    }

}

