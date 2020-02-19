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

    void ShaderTable::UploadToGPUMemory(uint8_t* uploadGPUMemory, const Buffer* gpuTableBuffer)
    {
        assert_format(gpuTableBuffer->RequestedMemory() >= mTableSize,
            "Buffer is not large enough to accommodate shader table. Query memory requirements first.");

        for (const auto& [shaderStage, records] : mRecords)
        {
            for (const Record& record : records)
            {
                memcpy(uploadGPUMemory + record.TableOffset, (uint8_t*)(&record.ID), sizeof(record.ID));

                // TODO: Write local root signature arguments here ??? 
                // Right now engine does not support local root signatures and arguments.
                // It will stay that way until a real need arises.
            }
        }

        mGPUTable = gpuTableBuffer;
    }

    void ShaderTable::Clear()
    {
        mRecords.clear();
        mPerStageStrides.clear();
        mTableSize = 0;
        mGPUTable = nullptr;
    }

    RayDispatchInfo ShaderTable::GenerateRayDispatchInfo() const
    {
        return {};
    }

    ShaderTable::MemoryRequirements ShaderTable::GetMemoryRequirements() const
    {
        return { mTableSize };
    }

    //std::optional<D3D12_GPU_VIRTUAL_ADDRESS> ShaderTable::ShaderStageFirstRecordAddress(Shader::Stage stage)
    //{
    //    /*    if (!mGPUTable) return std::nullopt;

    //        auto it = mRecords.find(stage);
    //        if (it == mRecords.end()) return std::nullopt;

    //        std::vector<Record>& records = it->second;
    //        if (records.empty()) return std::nullopt;*/

    //    return records.front().TableOffset * mGPUTable->GPUVirtualAddress();
    //}

}
