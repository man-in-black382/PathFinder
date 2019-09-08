#pragma once

#include <d3d12.h>
#include <memory>
#include <cstring>

#include "RootSignature.hpp"
#include "RingBufferResource.hpp"
#include "Shader.hpp"

namespace HAL
{

    class ShaderTable
    {
    public:
        using ShaderID = uint32_t;

        ShaderTable(const Device* device, uint8_t frameCapacity = 1);

        void AddShader(const Shader& shader, ShaderID id, const RootSignature* localRootSignature = nullptr);
        void UploadToGPUMemory();

        std::optional<D3D12_GPU_VIRTUAL_ADDRESS> ShaderStageFirstEntryAddress(Shader::Stage stage);


    private:
        struct TableEntry
        {
            TableEntry(ShaderID id, const RootSignature* signature, uint32_t tableOffset)
                : ID{ id }, Signature{ signature }, TableOffset{ tableOffset } {}

            ShaderID ID;
            const RootSignature* Signature;
            uint32_t TableOffset;
        };

        const Device* mDevice;
        uint8_t mFrameCapacity = 0;
        uint32_t mTableSize = 0;
        std::unique_ptr<RingBufferResource<uint8_t>> mGPUTable;
        std::unordered_map<Shader::Stage, std::vector<TableEntry>> mEntries;

    public:
        inline const auto GPUTable() const { return mGPUTable.get(); }
    };

}

