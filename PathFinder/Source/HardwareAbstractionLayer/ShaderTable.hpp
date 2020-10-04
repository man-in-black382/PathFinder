#pragma once

#include <d3d12.h>
#include <memory>
#include <cstring>
#include <limits>
#include <cstdint>

#include "RootSignature.hpp"
#include "Shader.hpp"
#include "RayDispatchInfo.hpp"
#include "Buffer.hpp"
#include "RayTracingHitGroupExport.hpp"

namespace HAL
{

    using ShaderTableIndex = uint32_t;

    class ShaderTable
    {
    public:
        struct MemoryRequirements
        {
            uint64_t TableSizeInBytes;
        };

        void SetRayGenerationShader(const ShaderIdentifier& id, const RootSignature* localRootSignature = nullptr);
        void AddRayMissShader(const ShaderIdentifier& id, const RootSignature* localRootSignature = nullptr);
        void AddCallableShader(const ShaderIdentifier& id, const RootSignature* localRootSignature = nullptr);
        void AddRayTracingHitGroupShaders(const ShaderIdentifier& hitGroupId, const RootSignature* localRootSignature = nullptr);

        RayDispatchInfo UploadToGPUMemory(uint8_t* uploadGPUMemory, const Buffer* gpuTableBuffer);
        void Clear();

        MemoryRequirements GetMemoryRequirements() const;

    private:
        struct ShaderRecord
        {
            ShaderRecord(const ShaderIdentifier& id, const RootSignature* signature, uint64_t size);

            ShaderIdentifier ID;
            const RootSignature* Signature;
            uint64_t SizeInBytes; 
        };

        void InsertNullRecordsWhereRequired();

        const Buffer* mGPUTable = nullptr;

        std::optional<ShaderRecord> mRayGenShaderRecord;
        std::vector<ShaderRecord> mRayMissShaderRecords;
        std::vector<ShaderRecord> mCallableShaderRecords;
        std::vector<ShaderRecord> mRayHitGroupRecords;

        // Shader table stride (maximum record size) for each RT shader type
        uint64_t mRayMissRecordStride = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
        uint64_t mRayHitGroupRecordStride = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
        uint64_t mCallableRecordStride = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
    };

}

