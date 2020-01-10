namespace PathFinder
{

    template <class T>
    void GPUCommandRecorder::BindExternalBuffer(const HAL::BufferResource<T>& resource, uint16_t shaderRegister, uint16_t registerSpace, HAL::ShaderRegister registerType)
    {
        mGraphicsDevice->BindExternalBuffer(resource, shaderRegister, registerSpace, registerType);
    }

    template <class T>
    void GPUCommandRecorder::SetRootConstants(const T& constants, uint16_t shaderRegister, uint16_t registerSpace)
    {
        mGraphicsDevice->SetRootConstants(constants, shaderRegister, registerSpace);
    }

}

