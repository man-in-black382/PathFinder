namespace PathFinder
{

    template <class T>
    void CommandRecorder::SetRootConstants(const T& constants, uint16_t shaderRegister, uint16_t registerSpace)
    {
        mGraphicsDevice->SetRootConstants(mPassGraph->Nodes()[mGraphNodeIndex], constants, shaderRegister, registerSpace);
    }

    template <size_t RTCount>
    void CommandRecorder::SetRenderTargets(const std::array<Foundation::Name, RTCount>& rtNames, std::optional<Foundation::Name> dsName)
    {
        mGraphicsDevice->SetRenderTargets(mPassGraph->Nodes()[mGraphNodeIndex], rtNames, dsName);
    }

}

