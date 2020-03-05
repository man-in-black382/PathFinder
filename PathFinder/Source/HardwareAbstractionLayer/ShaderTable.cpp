#include "ShaderTable.hpp"

#include <d3d12.h>
#include <limits>

#undef min
#undef max

namespace HAL
{

    ShaderTable::ShaderRecord::ShaderRecord(const ShaderIdentifier& id, const RootSignature* signature, uint64_t size)
        : ID{ id }, Signature{ signature }, SizeInBytes{ size } {}

    void ShaderTable::SetRayGenerationShader(const ShaderIdentifier& id, const RootSignature* localRootSignature)
    {
        uint64_t recordSize = sizeof(ShaderIdentifier);

        // Local root signature is optional
        if (localRootSignature)
        {
            // TODO: Implement correct root signature size calculation inside RootSignature itself
            //mTableSize += localRootSignature->ParameterCount() * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        }

        mRayGenShaderRecord = { id, localRootSignature, recordSize };
    }

    void ShaderTable::AddRayMissShader(const ShaderIdentifier& id, const RootSignature* localRootSignature)
    {
        uint64_t recordSize = sizeof(ShaderIdentifier);

        // Local root signature is optional
        if (localRootSignature)
        {
            // TODO: Implement correct root signature size calculation inside RootSignature itself
            //mTableSize += localRootSignature->ParameterCount() * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        }

        mRayMissShaderRecords.emplace_back(id, localRootSignature, recordSize);

        // We need to keep track of the largest shader record for it's stage, because DXR can only 
        // traverse table shaders of one type with a constant step (stride).
        // For example, if we have 3 miss shader records of size 40, 30 and 64 bytes, then the
        // stride should be 64 and all records must be aligned to 64 bytes when uploaded to GPU memory.
        // Each stage can have it's own stride though.
        mRayMissRecordStride = std::max(mRayMissRecordStride, recordSize);
    }

    void ShaderTable::AddRayTracingHitGroupShaders(const ShaderIdentifier& hitGroupId, const RootSignature* localRootSignature)
    {
        uint64_t recordSize = sizeof(ShaderIdentifier);

        if (localRootSignature)
        {
            // TODO: Implement correct root signature size calculation inside RootSignature itself
            //mTableSize += localRootSignature->ParameterCount() * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        }

        mRayHitGroupRecords.emplace_back(hitGroupId, localRootSignature, recordSize);
        mRayHitGroupRecordStride = std::max(mRayHitGroupRecordStride, recordSize);
    }

    RayDispatchInfo ShaderTable::UploadToGPUMemory(uint8_t* uploadGPUMemory, const Buffer* gpuTableBuffer)
    {
        D3D12_DISPATCH_RAYS_DESC addresses{};

        uint64_t recordAddressOffset = 0;

        assert_format(mRayGenShaderRecord, "Ray Generation shader is mandatory");

        // Upload ray generation table range
        addresses.RayGenerationShaderRecord = { recordAddressOffset, mRayGenShaderRecord->SizeInBytes };
        memcpy(uploadGPUMemory + recordAddressOffset, mRayGenShaderRecord->ID.RawData.data(), sizeof(mRayGenShaderRecord->ID.RawData));
        recordAddressOffset += mRayGenShaderRecord->SizeInBytes;

        // Upload miss shaders table region
        uint64_t missShaderRegionSize = mRayMissShaderRecords.size() * mRayMissRecordStride;
        addresses.MissShaderTable = { recordAddressOffset, missShaderRegionSize, mRayMissRecordStride };

        for (const ShaderRecord& missRecord : mRayMissShaderRecords)
        {
            memcpy(uploadGPUMemory + recordAddressOffset, missRecord.ID.RawData.data(), sizeof(missRecord.ID.RawData));
            recordAddressOffset += mRayMissRecordStride;
        }

        // Upload hit groups table region
        uint64_t hitGroupRegionSize = mRayHitGroupRecords.size() * mRayHitGroupRecordStride;
        addresses.HitGroupTable = { recordAddressOffset, hitGroupRegionSize, mRayHitGroupRecordStride };

        for (const ShaderRecord& hitGroupRecord : mRayHitGroupRecords)
        {
            memcpy(uploadGPUMemory + recordAddressOffset, hitGroupRecord.ID.RawData.data(), sizeof(hitGroupRecord.ID.RawData));
            recordAddressOffset += mRayHitGroupRecordStride;
        }

        mGPUTable = gpuTableBuffer;

        // Callable shaders are not yet supported. TODO: Implement callable shaders

        return RayDispatchInfo{ addresses };
    }

    void ShaderTable::Clear()
    {
        mRayGenShaderRecord = std::nullopt;
        mRayMissShaderRecords.clear();
        mRayHitGroupRecords.clear();
        mRayMissRecordStride = 0;
        mRayHitGroupRecordStride = 0;
        mGPUTable = nullptr;
    }

    ShaderTable::MemoryRequirements ShaderTable::GetMemoryRequirements() const
    {
        auto tableSize = mRayGenShaderRecord->SizeInBytes + 
            mRayMissRecordStride * mRayMissShaderRecords.size() +
            mRayHitGroupRecordStride * mRayHitGroupRecords.size();

        return { tableSize };
    }

}
