#include "ShaderExport.hpp"

#include <d3d12.h>

namespace HAL
{

    ShaderTable::ShaderTable(const Device* device, uint8_t frameCapacity)
        : mDevice{ device }, mFrameCapacity{ frameCapacity } {}

    void ShaderTable::AddShader(ShaderID id, const RootSignature* localRootSignature)
    {
        mEntries.emplace_back(id, localRootSignature);
    }

    void ShaderTable::UploadToGPUMemory()
    {

    }

}
