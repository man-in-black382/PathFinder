#pragma once

#include <d3d12.h>
#include <memory>
#include <cstring>
#include <limits>

#include "RootSignature.hpp"
#include "RingBufferResource.hpp"
#include "Shader.hpp"
#include "RayDispatchInfo.hpp"

namespace HAL
{

    class ShaderTable
    {
    public:
        using ShaderID = uint32_t;

        ShaderTable(const Device* device, uint8_t frameCapacity = 1);

        void AddShader(const Shader& shader, ShaderID id, const RootSignature* localRootSignature = nullptr);
        void UploadToGPUMemory();
        void Clear();

        RayDispatchInfo GenerateRayDispatchInfo() const;

        std::optional<D3D12_GPU_VIRTUAL_ADDRESS> ShaderStageFirstRecordAddress(Shader::Stage stage);

    private:
        struct Record
        {
            Record(ShaderID id, const RootSignature* signature, uint32_t tableOffset, uint32_t size)
                : ID{ id }, Signature{ signature }, TableOffset{ tableOffset }, SizeInBytes{ size } {}

            ShaderID ID;
            const RootSignature* Signature;
            uint32_t TableOffset; 
            uint32_t SizeInBytes; 
        };

        const Device* mDevice;
        uint8_t mFrameCapacity = 1;
        uint32_t mTableSize = 0;
        std::unique_ptr<RingBufferResource<uint8_t>> mGPUTable;
        std::unordered_map<Shader::Stage, std::vector<Record>> mRecords;

        /// Shader table stride (maximum record size) for each RT shader type
        std::unordered_map<Shader::Stage, uint32_t> mPerStageStrides;

    public:
        inline const auto GPUTable() const { return mGPUTable.get(); }
    };

}

