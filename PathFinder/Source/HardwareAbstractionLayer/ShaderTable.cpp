#include "ShaderTable.hpp"

#include <d3d12.h>
#include <limits>

#undef min
#undef max

namespace HAL
{

    ShaderTable::ShaderTable(const Device* device, uint8_t frameCapacity)
        : mDevice{ device }, mFrameCapacity{ frameCapacity } {}

    void ShaderTable::AddShader(const Shader& shader, ShaderID id, const RootSignature* localRootSignature)
    {
        uint32_t newRecordOffset = mTableSize;

        // Shader ID is always present in the table
        mTableSize += sizeof(ShaderID);

        // Local root signature is optional
        if (localRootSignature)
        {
            mTableSize += localRootSignature->ParameterCount() * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        }

        uint32_t size = mTableSize - newRecordOffset;

        mRecords[shader.PipelineStage()].emplace_back(id, localRootSignature, newRecordOffset, size);

        // We need to keep track of the largest shader record for it's stage, because DXR can only 
        // traverse table shaders of one type with a constant step (stride).
        // For example, if we have 3 miss shader records of size 40, 30 and 64 bytes, then the
        // stride should be 64 and all records must be aligned to 64 bytes when uploaded to GPU memory.
        // Each stage can have it's own stride though.
        
        mPerStageStrides[shader.PipelineStage()] = std::max(mPerStageStrides[shader.PipelineStage()], size);
    }

    void ShaderTable::UploadToGPUMemory()
    {
        mGPUTable = std::make_unique<RingBufferResource<uint8_t>>(
            *mDevice, mTableSize, mFrameCapacity, 1, CPUAccessibleHeapType::Upload);

        for (const auto& keyValue : mRecords)
        {
            const auto& records = keyValue.second;

            for (const Record& record : records)
            {
                // Write Shader ID
                mGPUTable->Write(record.TableOffset, (uint8_t*)(&record.ID), sizeof(record.ID));

                // TODO: Write local root signature arguments here ??? Probably not here
            }
        }
    }

    RayDispatchInfo ShaderTable::GenerateRayDispatchInfo() const
    {
        return {};
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
