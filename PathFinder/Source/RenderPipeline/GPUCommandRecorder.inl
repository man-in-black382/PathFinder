namespace PathFinder
{

    template <class T>
    void GPUCommandRecorder::SetRootConstants(const T& constants, uint16_t shaderRegister, uint16_t registerSpace)
    {
        mGraphicsDevice->SetRootConstants(constants, shaderRegister, registerSpace);
    }

}

