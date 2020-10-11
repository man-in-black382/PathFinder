#pragma once



namespace HAL
{
  
    template <class T>
    void ComputeCommandListBase::SetComputeRootConstants(const T& constants, uint32_t rootParameterIndex)
    {
        mList->SetComputeRoot32BitConstants(rootParameterIndex, sizeof(T) / 4, &constants, 0);
    }



    template <size_t RTCount>
    void GraphicsCommandListBase::SetRenderTargets(const std::array<const RTDescriptor*, RTCount>& rtDescriptors, const DSDescriptor* depthStencilDescriptor)
    {
        const D3D12_CPU_DESCRIPTOR_HANDLE* dsHandle = depthStencilDescriptor ? &depthStencilDescriptor->CPUHandle() : nullptr;
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE, RTCount> cpuHandles;
        std::transform(rtDescriptors.begin(), rtDescriptors.end(), cpuHandles.begin(), [](const RTDescriptor* rtd) { return rtd->CPUHandle(); });

        mList->OMSetRenderTargets(RTCount, cpuHandles.data(), false, dsHandle);
    }

    template <class T>
    void GraphicsCommandListBase::SetGraphicsRootConstants(const T& constants, uint32_t rootParameterIndex)
    {
        mList->SetGraphicsRoot32BitConstants(rootParameterIndex, sizeof(T) / 4, &constants, 0);
    }

}

