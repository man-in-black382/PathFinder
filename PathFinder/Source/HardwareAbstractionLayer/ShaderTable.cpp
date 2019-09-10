#include "ShaderTable.hpp"

#include <d3d12.h>

namespace HAL
{

    ShaderTable::ShaderTable(const Device* device, uint8_t frameCapacity)
        : mDevice{ device }, mFrameCapacity{ frameCapacity } {}

    void ShaderTable::AddShader(const Shader& shader, ShaderID id, const RootSignature* localRootSignature)
    {
        mRecords[shader.PipelineStage()].emplace_back(id, localRootSignature, mTableSize);
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

        for (const auto& keyValue : mRecords)
        {
            const auto& records = keyValue.second;

            for (const Record& record : records)
            {
                mGPUTable->Write(record.TableOffset, (uint8_t*)(&record.ID), sizeof(record.ID));
            }
        }
    }

    std::optional<D3D12_GPU_VIRTUAL_ADDRESS> ShaderTable::ShaderStageFirstRecordAddress(Shader::Stage stage)
    {
        if (!mGPUTable) return std::nullopt;

        auto it = mRecords.find(stage);
        if (it == mRecords.end()) return std::nullopt;
        
        std::vector<Record>& records = it->second;
        if (records.empty()) return std::nullopt;

        return records.front().TableOffset * mGPUTable->GPUVirtualAddress();
    }

}
