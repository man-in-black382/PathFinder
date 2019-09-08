#include "ShaderTable.hpp"

#include <d3d12.h>

namespace HAL
{

    ShaderTable::ShaderTable(const Device* device, uint8_t frameCapacity)
        : mDevice{ device }, mFrameCapacity{ frameCapacity } {}

    void ShaderTable::AddShader(const Shader& shader, ShaderID id, const RootSignature* localRootSignature)
    {
        mEntries.emplace(shader.PipelineStage(), id, localRootSignature, mTableSize);
        mTableSize += sizeof(ShaderID);

        if (localRootSignature)
        {
            mTableSize += localRootSignature->ParameterCount() * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        }
    }

    void ShaderTable::UploadToGPUMemory()
    {
        mGPUTable = std::make_unique<RingBufferResource<uint8_t>>(
            *mDevice, mTableSize, mFrameCapacity, 1, ResourceState::GenericRead, ResourceState::GenericRead, CPUAccessibleHeapType::Upload);

        for (const auto& keyValue : mEntries)
        {
            const auto& entries = keyValue.second;

            for (const ShaderTable::TableEntry& entry : entries)
            {
                mGPUTable->Write(entry.TableOffset, (uint8_t*)(&entry.ID), sizeof(entry.ID));
            }
        }
    }

    std::optional<D3D12_GPU_VIRTUAL_ADDRESS> ShaderTable::ShaderStageFirstEntryAddress(Shader::Stage stage)
    {
        auto it = mEntries.find(stage);
        if (it == mEntries.end()) return std::nullopt;
        
        std::vector<TableEntry>& entries = it->second;

    }

}
