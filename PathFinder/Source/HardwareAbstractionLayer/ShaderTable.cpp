#include "ShaderTable.hpp"

#include <Foundation/MemoryUtils.hpp>

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

        recordSize = Foundation::MemoryUtils::Align(recordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

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

        recordSize = Foundation::MemoryUtils::Align(recordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

        mRayMissShaderRecords.emplace_back(id, localRootSignature, recordSize);

        // We need to keep track of the largest shader record for it's stage, because DXR can only 
        // traverse table shaders of one type with a constant step (stride).
        // For example, if we have 3 miss shader records of size 40, 30 and 64 bytes, then the
        // stride should be 64 and all records must be aligned to 64 bytes when uploaded to GPU memory.
        // Each stage can have it's own stride though.
        mRayMissRecordStride = std::max(mRayMissRecordStride, recordSize);
    }

    void ShaderTable::AddCallableShader(const ShaderIdentifier& id, const RootSignature* localRootSignature)
    {
        uint64_t recordSize = sizeof(ShaderIdentifier);

        // Local root signature is optional
        if (localRootSignature)
        {
            // TODO: Implement correct root signature size calculation inside RootSignature itself
            //mTableSize += localRootSignature->ParameterCount() * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        }

        recordSize = Foundation::MemoryUtils::Align(recordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
        mCallableShaderRecords.emplace_back(id, localRootSignature, recordSize);
        mCallableRecordStride = std::max(mCallableRecordStride, recordSize);
    }

    void ShaderTable::AddRayTracingHitGroupShaders(const ShaderIdentifier& hitGroupId, const RootSignature* localRootSignature)
    {
        uint64_t recordSize = sizeof(ShaderIdentifier);

        if (localRootSignature)
        {
            // TODO: Implement correct root signature size calculation inside RootSignature itself
            //mTableSize += localRootSignature->ParameterCount() * sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
        }

        recordSize = Foundation::MemoryUtils::Align(recordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

        mRayHitGroupRecords.emplace_back(hitGroupId, localRootSignature, recordSize);
        mRayHitGroupRecordStride = std::max(mRayHitGroupRecordStride, recordSize);
    }

    RayDispatchInfo ShaderTable::UploadToGPUMemory(uint8_t* uploadGPUMemory, const Buffer* gpuTableBuffer)
    {
        D3D12_DISPATCH_RAYS_DESC addresses{};

        uint64_t recordAddressOffset = 0;

        assert_format(mRayGenShaderRecord, "Ray Generation shader is mandatory");

        InsertNullRecordsWhereRequired();

        // Upload ray generation table range
        auto rayGenTableStartFinalAddress = gpuTableBuffer->GPUVirtualAddress() + recordAddressOffset;
        auto rayGenTableStartUploadAddress = uploadGPUMemory + recordAddressOffset;

        addresses.RayGenerationShaderRecord = { rayGenTableStartFinalAddress, mRayGenShaderRecord->SizeInBytes };

        memcpy(rayGenTableStartUploadAddress, mRayGenShaderRecord->ID.RawData.data(), sizeof(mRayGenShaderRecord->ID.RawData));

        // Move address to Miss Table start and make sure it is aligned properly
        recordAddressOffset += mRayGenShaderRecord->SizeInBytes;
        recordAddressOffset = Foundation::MemoryUtils::Align(recordAddressOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

        if (!mRayMissShaderRecords.empty())
        {
            // Upload miss shaders table region
            uint64_t missShaderRegionSize = mRayMissShaderRecords.size() * mRayMissRecordStride;
            auto missTableStartFinalAddress = gpuTableBuffer->GPUVirtualAddress() + recordAddressOffset;
            addresses.MissShaderTable = { missTableStartFinalAddress, missShaderRegionSize, mRayMissRecordStride };
        }

        for (const ShaderRecord& missRecord : mRayMissShaderRecords)
        {
            auto missTableUploadAddress = uploadGPUMemory + recordAddressOffset;
            memcpy(missTableUploadAddress, missRecord.ID.RawData.data(), sizeof(missRecord.ID.RawData));
            recordAddressOffset += mRayMissRecordStride;
        }

        // We're done with miss shaders, address has moved past the last miss shader.
        // Now make sure it is properly aligned for Hit Group table records start
        recordAddressOffset = Foundation::MemoryUtils::Align(recordAddressOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

        if (!mRayHitGroupRecords.empty())
        {
            // Upload hit groups table region
            uint64_t hitGroupRegionSize = mRayHitGroupRecords.size() * mRayHitGroupRecordStride;
            auto hitGroupTableStartFinalAddress = gpuTableBuffer->GPUVirtualAddress() + recordAddressOffset;
            addresses.HitGroupTable = { hitGroupTableStartFinalAddress, hitGroupRegionSize, mRayHitGroupRecordStride };
        }

        for (const ShaderRecord& hitGroupRecord : mRayHitGroupRecords)
        {
            auto hitGroupTableUploadAddress = uploadGPUMemory + recordAddressOffset;
            memcpy(hitGroupTableUploadAddress, hitGroupRecord.ID.RawData.data(), sizeof(hitGroupRecord.ID.RawData));
            recordAddressOffset += mRayHitGroupRecordStride;
        }

        // We're done with hit groups, address has moved past the last hit group shader.
        // Alight it for callable table records start address
        recordAddressOffset = Foundation::MemoryUtils::Align(recordAddressOffset, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

        if (!mCallableShaderRecords.empty())
        {
            uint64_t callableShadersRegionSize = mCallableShaderRecords.size() * mCallableRecordStride;
            auto callableTableStartFinalAddress = gpuTableBuffer->GPUVirtualAddress() + recordAddressOffset;
            addresses.CallableShaderTable = { callableTableStartFinalAddress, callableShadersRegionSize, mCallableRecordStride };
        }

        for (const ShaderRecord& callableRecord : mCallableShaderRecords)
        {
            auto callableTableUploadAddress = uploadGPUMemory + recordAddressOffset;
            memcpy(callableTableUploadAddress, callableRecord.ID.RawData.data(), sizeof(callableRecord.ID.RawData));
            recordAddressOffset += mCallableRecordStride;
        }

        mGPUTable = gpuTableBuffer;

        return RayDispatchInfo{ addresses };
    }

    void ShaderTable::Clear()
    {
        mRayGenShaderRecord = std::nullopt;
        mRayMissShaderRecords.clear();
        mRayHitGroupRecords.clear();
        mCallableShaderRecords.clear();
        mRayMissRecordStride = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
        mRayHitGroupRecordStride = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
        mCallableRecordStride = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
        mGPUTable = nullptr;
    }

    ShaderTable::MemoryRequirements ShaderTable::GetMemoryRequirements() const
    {
        // Shader tables must be aligned
        // Reserve at least 1 slot for each record type 
        auto tableSize = Foundation::MemoryUtils::Align(mRayGenShaderRecord->SizeInBytes, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) +
            Foundation::MemoryUtils::Align(mRayMissRecordStride * std::max(mRayMissShaderRecords.size(), (size_t)1), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) +
            Foundation::MemoryUtils::Align(mCallableRecordStride * std::max(mCallableShaderRecords.size(), (size_t)1), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) +
            Foundation::MemoryUtils::Align(mRayHitGroupRecordStride * std::max(mRayHitGroupRecords.size(), (size_t)1), D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

        return { tableSize };
    }

    void ShaderTable::InsertNullRecordsWhereRequired()
    {
        if (mRayMissShaderRecords.empty()) AddRayMissShader(ShaderIdentifier(), nullptr);
        if (mCallableShaderRecords.empty()) AddCallableShader(ShaderIdentifier(), nullptr);
        if (mRayHitGroupRecords.empty()) AddRayTracingHitGroupShaders(ShaderIdentifier(), nullptr);
    }

}
